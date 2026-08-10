#ifndef TENSOR_NETWORK_SIMULATOR_HPP
#define TENSOR_NETWORK_SIMULATOR_HPP

#include <Eigen/Dense>
#include <complex>
#include <iostream>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "utils.hpp"
#include "QuantumVector.hpp"

// Tensor-network backend for MBQC simulation.
//
// Qubit indices ("positions") behave exactly like in StatevectorSimulator:
// they are a contiguous compact numbering [0, num_qubits), where a freshly
// added qubit takes position 0 and all others shift up by one, and tracing
// a qubit out shifts everything above it down by one.
//
// Internally, every entangled cluster of qubits ("block") is stored as a
// small Matrix Product State (MPS) chain rather than one dense amplitude
// vector: a sequence of rank-3 site tensors, one per qubit in the block,
// linked by "bond" indices whose dimension reflects how entangled the two
// sides of that cut are. Unentangled qubits therefore cost O(1) (a
// single 1x1-bonded site) and a CZ between two qubits only grows the bond
// it actually touches - blocks with limited entanglement stay small no
// matter how many qubits they contain, instead of every block being a
// dense 2^numQubits vector. maxBondDim (chi) caps how large a bond is
// allowed to grow via SVD truncation; chi = 0 means "no cap", i.e. every
// bond is kept at its full (exact) Schmidt rank and the simulation stays
// mathematically exact, just no longer forced through a dense vector.

class TensorNetworkSimulator {
public:
    using cplx = std::complex<double>;
    using VectorC = Eigen::VectorXcd;
    using Matrix2C = Eigen::Matrix2cd;
    using Matrix4C = Eigen::Matrix4cd;
    using MatrixC = Eigen::MatrixXcd;

private:
    // A single site of an MPS chain: physical dimension 2 (one qubit),
    // bond dimension leftDim() on the left, rightDim() on the right.
    // data[p] is the (leftDim x rightDim) matrix of amplitudes for
    // physical value p. Boundary sites of a block always have
    // leftDim()==1 (leftmost) / rightDim()==1 (rightmost).
    struct SiteTensor {
        MatrixC data[2];
        int leftDim() const { return (int)data[0].rows(); }
        int rightDim() const { return (int)data[0].cols(); }
    };

    // An entangled cluster of qubits, stored as an MPS chain. sites[i] is
    // the qubit currently at local chain position i (see Loc::localBit
    // below) - chain position doubles as the "bit position" convention
    // used everywhere else in this file (position 0 = LSB).
    struct Block {
        std::vector<SiteTensor> sites;
        int numQubits() const { return (int)sites.size(); }
    };

    // Which block a given global qubit position currently lives in, and at
    // which local chain position inside that block.
    struct Loc {
        int blockId;
        int localBit;
    };

    std::unordered_map<int, Block> blocks;
    int nextBlockId = 0;
    std::vector<Loc> posToLoc;   // posToLoc[globalPos] -> location

    bool randomMeasurements = true;
    std::mt19937 rng;

    // Maximum bond dimension allowed when a two-qubit gate reshapes a
    // block's internal bonds. 0 means unbounded (exact simulation): SVD
    // still discards numerically-zero singular values, but never drops
    // any that carry real weight, so no approximation is introduced.
    int maxBondDim = 0;

    // Running estimate of the fidelity lost to bond truncation so far:
    // the product, over every SVD split that actually discarded
    // non-negligible singular-value weight, of the retained probability
    // fraction at that split. It is exact (stays at 1.0) whenever
    // maxBondDim == 0, since nothing with real weight is ever dropped in
    // that mode. When maxBondDim > 0 this is the standard TEBD-style
    // truncation-error estimate: each split's retained fraction is exact
    // *locally*, but since the chain isn't kept in full canonical form
    // between operations, the product is an estimate of the true global
    // fidelity |<psi_exact|psi_truncated>|^2 rather than an exact value.
    double fidelityEstimate = 1.0;

    int newBlockId() { return nextBlockId++; }

    static SiteTensor makeBasisSite(cplx amp0, cplx amp1) {
        SiteTensor s;
        s.data[0] = MatrixC::Constant(1, 1, amp0);
        s.data[1] = MatrixC::Constant(1, 1, amp1);
        return s;
    }

    static void applySingleSiteGate(SiteTensor& s, const Matrix2C& gate) {
        MatrixC new0 = gate(0,0) * s.data[0] + gate(0,1) * s.data[1];
        MatrixC new1 = gate(1,0) * s.data[0] + gate(1,1) * s.data[1];
        s.data[0] = std::move(new0);
        s.data[1] = std::move(new1);
    }

    // Contracts two neighbouring sites L, R into theta[p,q], applies a
    // 4x4 two-qubit gate G (row/col index = 2*p+q, p = L's physical
    // index, q = R's physical index), then splits the result back into L
    // and R via SVD, truncating the new shared bond to at most
    // maxBondDim (0 = keep the full Schmidt rank, i.e. exact). Used for
    // both real gates (CZ) and the SWAP moves that bring non-adjacent
    // qubits together. Returns the fraction of this split's singular-value
    // weight that was retained (1.0 if nothing with real weight was
    // dropped), for the caller to fold into a running fidelity estimate.
    static double applyTwoSiteGate(SiteTensor& L, SiteTensor& R, const Matrix4C& G, int maxBondDim) {
        int dl = L.leftDim();
        int dr = R.rightDim();

        MatrixC theta[2][2];
        for (int p = 0; p < 2; ++p)
            for (int q = 0; q < 2; ++q)
                theta[p][q] = L.data[p] * R.data[q]; // dl x dr

        MatrixC newTheta[2][2];
        for (int pp = 0; pp < 2; ++pp) {
            for (int qq = 0; qq < 2; ++qq) {
                MatrixC acc = MatrixC::Zero(dl, dr);
                int row = pp * 2 + qq;
                for (int p = 0; p < 2; ++p) {
                    for (int q = 0; q < 2; ++q) {
                        cplx g = G(row, p * 2 + q);
                        if (g != cplx(0.0, 0.0)) acc += g * theta[p][q];
                    }
                }
                newTheta[pp][qq] = std::move(acc);
            }
        }

        MatrixC M(2 * dl, 2 * dr);
        for (int pp = 0; pp < 2; ++pp)
            for (int qq = 0; qq < 2; ++qq)
                M.block(pp * dl, qq * dr, dl, dr) = newTheta[pp][qq];

        Eigen::JacobiSVD<MatrixC> svd(M, Eigen::ComputeThinU | Eigen::ComputeThinV);
        const Eigen::VectorXd& S = svd.singularValues();
        int total = (int)S.size();
        double sigma0 = total > 0 ? S(0) : 0.0;
        double eps = std::max(1e-13, sigma0 * 1e-12);

        int rank = 0;
        for (int i = 0; i < total; ++i) if (S(i) > eps) ++rank;
        rank = std::max(rank, 1);
        int keep = (maxBondDim > 0) ? std::min(rank, maxBondDim) : rank;
        keep = std::min(keep, std::max(total, 1));
        keep = std::max(keep, 1);

        double normKeptSq = 0.0;
        for (int i = 0; i < keep; ++i) normKeptSq += S(i) * S(i);
        double totalNormSq = 0.0;
        for (int i = 0; i < total; ++i) totalNormSq += S(i) * S(i);
        double normKept = std::sqrt(normKeptSq);
        // Only rescale when maxBondDim actually forced us to cut below
        // the true rank. total is just min(rows,cols) from the thin SVD
        // and is routinely larger than rank (e.g. dl,dr bigger than the
        // real Schmidt rank at this cut) - dropping those extra
        // numerically-zero singular values is lossless and must NOT be
        // rescaled, or every such (very common) split silently corrupts
        // the state's global norm.
        bool realTruncation = (maxBondDim > 0) && (keep < rank);
        double scale = (realTruncation && normKept > 1e-300) ? (1.0 / normKept) : 1.0;
        double retained = (realTruncation && totalNormSq > 1e-300) ? (normKeptSq / totalNormSq) : 1.0;

        MatrixC U = svd.matrixU().leftCols(keep);            // 2dl x keep
        MatrixC Vt = svd.matrixV().leftCols(keep).adjoint();  // keep x 2dr

        L.data[0] = U.block(0, 0, dl, keep);
        L.data[1] = U.block(dl, 0, dl, keep);

        MatrixC SV(keep, 2 * dr);
        for (int i = 0; i < keep; ++i) SV.row(i) = scale * S(i) * Vt.row(i);
        R.data[0] = SV.block(0, 0, keep, dr);
        R.data[1] = SV.block(0, dr, keep, dr);

        return retained;
    }

    static const Matrix4C& czGate() {
        static const Matrix4C G = (Matrix4C() <<
            1,0,0, 0,
            0,1,0, 0,
            0,0,1, 0,
            0,0,0,-1).finished();
        return G;
    }

    static const Matrix4C& swapGate() {
        static const Matrix4C G = (Matrix4C() <<
            1,0,0,0,
            0,0,1,0,
            0,1,0,0,
            0,0,0,1).finished();
        return G;
    }

    // Contracts a block's MPS chain into a plain dense amplitude vector
    // (chain position i = bit i, position 0 = LSB). Exponential in the
    // block's qubit count - only meant for small blocks or external
    // readout (see contractAll).
    static VectorC blockToDense(const Block& block) {
        int nb = block.numQubits();
        if (nb == 0) return VectorC();

        int rows = 1;
        MatrixC acc(1, 1);
        acc(0, 0) = cplx(1.0, 0.0);
        for (int k = 0; k < nb; ++k) {
            const SiteTensor& s = block.sites[k];
            int newRows = rows * 2;
            MatrixC newAcc(newRows, s.rightDim());
            for (int cfg = 0; cfg < rows; ++cfg) {
                for (int p = 0; p < 2; ++p) {
                    newAcc.row(cfg | (p << k)) = acc.row(cfg) * s.data[p];
                }
            }
            acc = std::move(newAcc);
            rows = newRows;
        }

        VectorC result(rows);
        for (int i = 0; i < rows; ++i) result(i) = acc(i, 0);
        return result;
    }

    // Inverse of blockToDense: decomposes a dense 2^n amplitude vector
    // into an n-site MPS chain via a left-to-right sweep of SVDs (bit 0 =
    // LSB = site 0), truncating each new bond to maxBondDim (0 = exact).
    // Standard "state vector -> MPS" construction. If fidelityInOut is
    // given, each split's retained fraction (see applyTwoSiteGate) is
    // multiplied into it, same convention as the running fidelityEstimate.
    static Block denseToBlock(const VectorC& state, int n, int maxBondDim, double* fidelityInOut = nullptr) {
        Block block;
        if (n == 0) return block;
        block.sites.reserve(n);

        int leftDim = 1;
        MatrixC M(1, (Eigen::Index)1 << n);
        M.row(0) = state.transpose();

        for (int k = 0; k < n; ++k) {
            int remaining = n - k;
            int dim = 1 << remaining;
            int halfDim = dim / 2;

            MatrixC M2(leftDim * 2, halfDim);
            for (int l = 0; l < leftDim; ++l) {
                for (int col = 0; col < dim; ++col) {
                    int p = col & 1;
                    int rest = col >> 1;
                    M2(p * leftDim + l, rest) = M(l, col);
                }
            }

            Eigen::JacobiSVD<MatrixC> svd(M2, Eigen::ComputeThinU | Eigen::ComputeThinV);
            const Eigen::VectorXd& S = svd.singularValues();
            int total = (int)S.size();
            double sigma0 = total > 0 ? S(0) : 0.0;
            double eps = std::max(1e-13, sigma0 * 1e-12);

            int rank = 0;
            for (int i = 0; i < total; ++i) if (S(i) > eps) ++rank;
            rank = std::max(rank, 1);
            int keep = (maxBondDim > 0) ? std::min(rank, maxBondDim) : rank;
            keep = std::min(keep, std::max(total, 1));
            keep = std::max(keep, 1);

            double normKeptSq = 0.0;
            for (int i = 0; i < keep; ++i) normKeptSq += S(i) * S(i);
            double totalNormSq = 0.0;
            for (int i = 0; i < total; ++i) totalNormSq += S(i) * S(i);
            double normKept = std::sqrt(normKeptSq);
            // See applyTwoSiteGate for why this must be keep < rank, not
            // keep < total.
            bool realTruncation = (maxBondDim > 0) && (keep < rank);
            double scale = (realTruncation && normKept > 1e-300) ? (1.0 / normKept) : 1.0;
            if (fidelityInOut && realTruncation && totalNormSq > 1e-300) {
                *fidelityInOut *= (normKeptSq / totalNormSq);
            }

            MatrixC U = svd.matrixU().leftCols(keep);
            SiteTensor site;
            site.data[0] = U.block(0, 0, leftDim, keep);
            site.data[1] = U.block(leftDim, 0, leftDim, keep);
            block.sites.push_back(std::move(site));

            MatrixC Vt = svd.matrixV().leftCols(keep).adjoint(); // keep x halfDim
            MatrixC next(keep, halfDim);
            for (int i = 0; i < keep; ++i) next.row(i) = scale * S(i) * Vt.row(i);
            M = std::move(next);
            leftDim = keep;
        }

        return block;
    }

    struct MergedRef { int blockId; int localU; int localV; };

    // Ensures the qubits at global positions u and v share a single block,
    // merging their two blocks if they don't already, and returns that
    // block together with u and v's local chain positions inside it.
    // Merging is just chain concatenation (A's sites followed by B's) -
    // two independent blocks are, by construction, in a product state, so
    // no contraction/SVD is needed to fuse their chains; entanglement (and
    // the associated bond growth) only happens once the caller actually
    // applies a two-qubit gate across the seam.
    MergedRef ensureSameBlock(int posU, int posV) {
        Loc lu = posToLoc[posU];
        Loc lv = posToLoc[posV];
        if (lu.blockId == lv.blockId) {
            return {lu.blockId, lu.localBit, lv.localBit};
        }

        Block& A = blocks.at(lu.blockId);
        Block& B = blocks.at(lv.blockId);
        int nA = A.numQubits();

        Block merged;
        merged.sites = std::move(A.sites);
        merged.sites.reserve(merged.sites.size() + B.sites.size());
        for (auto& s : B.sites) merged.sites.push_back(std::move(s));

        int oldIdU = lu.blockId, oldIdV = lv.blockId;
        int newId = newBlockId();
        for (auto& loc : posToLoc) {
            if (loc.blockId == oldIdU) {
                loc.blockId = newId;
            } else if (loc.blockId == oldIdV) {
                loc.blockId = newId;
                loc.localBit += nA;
            }
        }

        blocks.erase(oldIdU);
        blocks.erase(oldIdV);
        blocks.emplace(newId, std::move(merged));

        return {newId, lu.localBit, lv.localBit + nA};
    }

    // Folds every currently active qubit into a single block, with
    // position i ending up at chain position i (matches StatevectorSimulator's
    // flat layout). Used by setState/setStateSubsystem, which - like their
    // StatevectorSimulator counterparts - need one flat amplitude vector
    // to write into and are only ever called right after construction.
    int mergeAllIntoOneBlock() {
        int n = (int)posToLoc.size();
        if (n == 0) return -1;
        for (int i = 1; i < n; ++i) ensureSameBlock(0, i);
        return posToLoc[0].blockId;
    }

    // Contracts every block into one dense amplitude vector ordered by
    // canonical global position (position 0 = LSB), i.e. exactly the
    // layout StatevectorSimulator would produce. Only used for external
    // readout (getStatevectorBraKet / toJson) - never on the hot
    // simulation path, so its exponential cost is acceptable there.
    VectorC contractAll() const {
        int n = (int)posToLoc.size();
        if (n == 0) return VectorC();

        VectorC temp(1);
        temp(0) = cplx(1.0, 0.0);
        std::vector<int> tempBitOfGlobalPos(n, -1);
        int bitsSoFar = 0;

        for (const auto& [id, block] : blocks) {
            std::vector<int> positions(block.numQubits(), -1);
            for (int p = 0; p < n; ++p) {
                if (posToLoc[p].blockId == id) positions[posToLoc[p].localBit] = p;
            }

            VectorC blockAmp = blockToDense(block);

            int dimOld = (int)temp.size();
            int dimBlock = (int)blockAmp.size();
            VectorC next = VectorC::Zero(dimOld * dimBlock);
            for (int i = 0; i < dimOld; ++i) {
                cplx c = temp(i);
                if (c == cplx(0.0, 0.0)) continue;
                for (int j = 0; j < dimBlock; ++j) {
                    next(i | (j << bitsSoFar)) = c * blockAmp(j);
                }
            }
            temp = std::move(next);

            for (int localBit = 0; localBit < block.numQubits(); ++localBit) {
                tempBitOfGlobalPos[positions[localBit]] = bitsSoFar + localBit;
            }
            bitsSoFar += block.numQubits();
        }

        int dim = 1 << n;
        VectorC result = VectorC::Zero(dim);
        for (int idx = 0; idx < dim; ++idx) {
            int newIdx = 0;
            for (int g = 0; g < n; ++g) {
                int bit = get_bit(idx, tempBitOfGlobalPos[g]);
                newIdx = set_bit(newIdx, g, bit);
            }
            result(newIdx) = temp(idx);
        }
        return result;
    }

public:
    TensorNetworkSimulator() : TensorNetworkSimulator(0, true) {}
    TensorNetworkSimulator(int n, bool random = true, int maxBondDim = 0)
        : randomMeasurements(random), rng(std::random_device{}()), maxBondDim(maxBondDim) {
        if (n < 0) throw std::invalid_argument("Number of qubits must be non-negative");
        if (maxBondDim < 0) throw std::invalid_argument("maxBondDim must be non-negative (0 = no truncation)");
        // Each qubit starts in its own 1-qubit block (|0>), not pre-merged
        // into one n-qubit block, so unentangled qubits never pay for a
        // dense joint amplitude vector until a CZ actually links them.
        posToLoc.resize(n);
        for (int i = 0; i < n; ++i) {
            Block b;
            b.sites.push_back(makeBasisSite(cplx(1.0, 0.0), cplx(0.0, 0.0)));
            int id = newBlockId();
            blocks.emplace(id, std::move(b));
            posToLoc[i] = Loc{id, 0};
        }
    }

    int get_num_qubits() const { return (int)posToLoc.size(); }

    int getMaxBondDim() const { return maxBondDim; }
    void setMaxBondDim(int chi) {
        if (chi < 0) throw std::invalid_argument("maxBondDim must be non-negative (0 = no truncation)");
        maxBondDim = chi;
    }

    // Running estimate of |<psi_exact|psi_truncated>|^2 accumulated since
    // construction/reset - see the fidelityEstimate member comment for
    // exactly what "estimate" means here. Always exactly 1.0 when
    // maxBondDim == 0.
    double getFidelityEstimate() const { return fidelityEstimate; }

    VectorC get_statevector() const { return contractAll(); }

    // Returns, for each currently active tensor block, the sorted list of
    // global qubit positions living in it. Blocks are ordered by the
    // smallest position they contain. Two positions end up in the same
    // block iff their qubits have become entangled (directly or
    // transitively) via CZ - this is the actual unit of dense storage in
    // this backend, so it's the right thing to visualize/inspect.
    std::vector<std::vector<int>> getBlockStructure() const {
        std::unordered_map<int, std::vector<int>> byBlock;
        for (int pos = 0; pos < (int)posToLoc.size(); ++pos) {
            byBlock[posToLoc[pos].blockId].push_back(pos);
        }
        std::vector<std::vector<int>> result;
        result.reserve(byBlock.size());
        for (auto& [id, positions] : byBlock) {
            std::sort(positions.begin(), positions.end());
            result.push_back(std::move(positions));
        }
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.front() < b.front();
        });
        return result;
    }

    // Which block a given global qubit position currently lives in - the
    // id to pass to getSingularValueSpectrum below.
    int getBlockId(int qubit) const {
        if (qubit < 0 || qubit >= (int)posToLoc.size())
            throw std::out_of_range("getBlockId: qubit index out of range");
        return posToLoc[qubit].blockId;
    }

    // Diagnostic only: the Schmidt singular-value spectrum across every
    // internal cut of the given block's MPS chain (cut i separates chain
    // positions [0..i] from [i+1..end), so a block of nb qubits has nb-1
    // cuts), sorted descending within each cut - exactly the values an
    // SVD-truncated MPS would have to compress at that bond. Computed by
    // contracting the block to a dense vector and reshaping around each
    // cut, so it is exponential in the block's qubit count; only meant
    // for small/medium blocks (e.g. via getBlockStructure), never on the
    // hot simulation path.
    std::vector<std::vector<double>> getSingularValueSpectrum(int blockId) const {
        auto it = blocks.find(blockId);
        if (it == blocks.end()) throw std::out_of_range("getSingularValueSpectrum: no such block");
        const Block& block = it->second;
        int nb = block.numQubits();

        VectorC dense = blockToDense(block);
        std::vector<std::vector<double>> spectra;
        spectra.reserve(std::max(nb - 1, 0));
        for (int cut = 0; cut < nb - 1; ++cut) {
            int leftDim = 1 << (cut + 1);
            int rightDim = 1 << (nb - cut - 1);
            MatrixC M(leftDim, rightDim);
            for (Eigen::Index idx = 0; idx < dense.size(); ++idx) {
                int l = (int)idx & (leftDim - 1);
                int r = (int)idx >> (cut + 1);
                M(l, r) = dense(idx);
            }
            Eigen::JacobiSVD<MatrixC> svd(M);
            const Eigen::VectorXd& S = svd.singularValues();
            spectra.emplace_back(S.data(), S.data() + S.size());
        }
        return spectra;
    }

    // Total number of amplitudes actually stored across all blocks, i.e.
    // the sum over every MPS site tensor of 2 * leftDim * rightDim - the
    // real memory footprint of this backend, as opposed to the
    // 2^numQubits a dense statevector would need. Reflects how well the
    // MPS chains are compressing the state (which shrinks as maxBondDim
    // shrinks, at the cost of approximation).
    long long getStoredAmplitudeCount() const {
        long long total = 0;
        for (const auto& [id, block] : blocks)
            for (const auto& s : block.sites)
                total += 2LL * s.leftDim() * s.rightDim();
        return total;
    }

    void reset() {
        int n = (int)posToLoc.size();
        blocks.clear();
        posToLoc.clear();
        posToLoc.resize(n);
        for (int i = 0; i < n; ++i) {
            Block b;
            b.sites.push_back(makeBasisSite(cplx(1.0, 0.0), cplx(0.0, 0.0)));
            int id = newBlockId();
            blocks.emplace(id, std::move(b));
            posToLoc[i] = Loc{id, 0};
        }
        fidelityEstimate = 1.0;
    }

    std::string getStatevectorBraKet() const {
        return vectorToBraKet(contractAll());
    }

    json toJson() const {
        return vectorToJson(contractAll());
    }

    static VectorC parseBraKet(const std::string& braket) {
        return parseBraKetVector(braket);
    }

    // Like StatevectorSimulator, only valid right after construction while
    // all qubits still form a single fresh |00...0> block.
    void setState(const VectorC& state) {
        int n = (int)posToLoc.size();
        int expected_state_size = 1 << n;
        if (state.size() != expected_state_size)
            throw std::invalid_argument("State vector size must be 2^num_qubits");
        if (n == 0) return;

        int blockId = mergeAllIntoOneBlock();
        Block& block = blocks.at(blockId);
        VectorC current = blockToDense(block);

        if (std::abs(current(0) - cplx(1.0, 0.0)) > TOLERANCE)
            throw std::runtime_error("setState only works on reset statevector |00...0>");
        for (int i = 1; i < current.size(); ++i)
            if (std::abs(current(i)) > TOLERANCE)
                throw std::runtime_error("setState only works on reset statevector |00...0>");

        block.sites = denseToBlock(state, n, maxBondDim, &fidelityEstimate).sites;
    }

    void setStateSubsystem(const std::vector<int>& qubits, const VectorC& state) {
        if (qubits.empty()) throw std::invalid_argument("Qubit list cannot be empty");
        int subregister_size = static_cast<int>(qubits.size());
        int expected_state_size = 1 << subregister_size;
        if (state.size() != expected_state_size) throw std::invalid_argument("State vector size must be 2^(number of qubits)");

        int n = (int)posToLoc.size();
        if (n == 0) throw std::out_of_range("setStateSubsystem: no qubits present");
        int blockId = mergeAllIntoOneBlock();
        Block& block = blocks.at(blockId);
        VectorC current = blockToDense(block);

        if (std::abs(current(0) - cplx(1.0, 0.0)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");
        for (int i = 1; i < current.size(); ++i) if (std::abs(current(i)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");

        std::vector<int> sorted_qubits = qubits;
        std::sort(sorted_qubits.begin(), sorted_qubits.end());
        for (size_t i = 0; i < sorted_qubits.size(); ++i) {
            if (sorted_qubits[i] < 0 || sorted_qubits[i] >= n) throw std::out_of_range("TensorNetwork setState: Qubit index out of range");
            if (i > 0 && sorted_qubits[i] == sorted_qubits[i-1]) throw std::invalid_argument("Duplicate qubit indices not allowed");
        }

        VectorC full = VectorC::Zero(1 << n);
        for (int sub_idx = 0; sub_idx < expected_state_size; ++sub_idx) {
            int full_idx = 0;
            for (int i = 0; i < subregister_size; ++i) {
                int bit = (sub_idx >> i) & 1;
                full_idx = set_bit(full_idx, qubits[i], bit);
            }
            full(full_idx) = state(sub_idx);
        }
        block.sites = denseToBlock(full, n, maxBondDim, &fidelityEstimate).sites;
    }

    // New qubit takes position 0; every previously active qubit's position shifts up by one
    int add_qubit_plus() {
        Block b;
        const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
        b.sites.push_back(makeBasisSite(cplx(inv_sqrt2, 0.0), cplx(inv_sqrt2, 0.0)));

        int retIndex = (int)posToLoc.size();
        int id = newBlockId();
        blocks.emplace(id, std::move(b));
        posToLoc.insert(posToLoc.begin(), Loc{id, 0});
        return retIndex;
    }

    // permutation[new_qubit_index] = old_qubit_index. Blocks themselves are
    // never touched - only the bookkeeping of which global position points
    // at which block/local-bit changes, so this is O(n) instead of the
    // O(2^n) a dense statevector needs for the same operation.
    void reorderQubits(const std::vector<int>& permutation) {
        int n = (int)posToLoc.size();
        if ((int)permutation.size() != n)
            throw std::invalid_argument("reorderQubits: permutation size mismatch");

        std::vector<Loc> new_posToLoc(n);
        for (int new_q = 0; new_q < n; ++new_q)
            new_posToLoc[new_q] = posToLoc[permutation[new_q]];
        posToLoc = std::move(new_posToLoc);
    }

    // ============== GATES ==============
    void apply_single_qubit_gate(int qubit, const Matrix2C& gate) {
        if (qubit < 0 || qubit >= (int)posToLoc.size()) throw std::out_of_range("TensorNetwork SingleQgate: Qubit index out of range");
        Loc loc = posToLoc[qubit];
        applySingleSiteGate(blocks.at(loc.blockId).sites[loc.localBit], gate);
    }

    void X(int qubit) {
        Matrix2C g; g << 0.0, 1.0,
                         1.0, 0.0;
        apply_single_qubit_gate(qubit, g);
    }

    void Y(int qubit) {
        Matrix2C g; g << 0.0, cplx(0,-1),
                         cplx(0,1), 0.0;
        apply_single_qubit_gate(qubit, g);
    }

    void Z(int qubit) {
        Matrix2C g; g << 1.0, 0.0,
                         0.0, -1.0;
        apply_single_qubit_gate(qubit, g);
    }

    void S(int qubit) {
        Matrix2C g; g << 1.0, 0.0,
                         0.0, cplx(0,1);
        apply_single_qubit_gate(qubit, g);
    }

    void Sdg(int qubit) {
        Matrix2C g; g << 1.0, 0.0,
                         0.0, cplx(0,-1);
        apply_single_qubit_gate(qubit, g);
    }

    void H(int qubit) {
        double s = 1.0 / std::sqrt(2.0);
        Matrix2C g; g << s, s,
                         s, -s;
        apply_single_qubit_gate(qubit, g);
    }

    void CZ(int control, int target) {
        int n = (int)posToLoc.size();
        if (control < 0 || control >= n || target < 0 || target >= n) throw std::out_of_range("TensorNetwork CZ: Qubit index out of range");
        if (control == target) throw std::invalid_argument("Control and target qubits must be different");

        MergedRef ref = ensureSameBlock(control, target);
        Block& block = blocks.at(ref.blockId);
        int nb = block.numQubits();

        // control/target need not be adjacent in the chain (nor even in
        // the same block before the merge above) - walk a swap network to
        // bring them next to each other, remembering which global qubit
        // ends up at each chain position so posToLoc can be fixed up once
        // at the end instead of on every individual swap.
        std::vector<int> chainQubit(nb, -1);
        for (int p = 0; p < n; ++p)
            if (posToLoc[p].blockId == ref.blockId) chainQubit[posToLoc[p].localBit] = p;

        int lo = std::min(ref.localU, ref.localV);
        int hi = std::max(ref.localU, ref.localV);
        for (int k = hi; k > lo + 1; --k) {
            fidelityEstimate *= applyTwoSiteGate(block.sites[k - 1], block.sites[k], swapGate(), maxBondDim);
            std::swap(chainQubit[k - 1], chainQubit[k]);
        }

        fidelityEstimate *= applyTwoSiteGate(block.sites[lo], block.sites[lo + 1], czGate(), maxBondDim);

        for (int k = 0; k < nb; ++k) posToLoc[chainQubit[k]].localBit = k;
    }

    // ============== MEASUREMENTS ==============

    // Reduced single-qubit probability p(bit=0) for the site at chain
    // position siteIdx, computed via left/right environment contraction
    // (O(numQubits * chi^3)) instead of materializing the full 2^n dense
    // vector. Gauge-independent - works regardless of canonical form.
    static double computeP0(const Block& block, int siteIdx, double& outTotalNorm) {
        int nb = block.numQubits();

        MatrixC L = MatrixC::Identity(block.sites[0].leftDim(), block.sites[0].leftDim());
        for (int k = 0; k < siteIdx; ++k) {
            const SiteTensor& s = block.sites[k];
            MatrixC newL = MatrixC::Zero(s.rightDim(), s.rightDim());
            for (int p = 0; p < 2; ++p) newL += s.data[p].adjoint() * L * s.data[p];
            L = std::move(newL);
        }

        MatrixC R = MatrixC::Identity(block.sites[nb - 1].rightDim(), block.sites[nb - 1].rightDim());
        for (int k = nb - 1; k > siteIdx; --k) {
            const SiteTensor& s = block.sites[k];
            MatrixC newR = MatrixC::Zero(s.leftDim(), s.leftDim());
            for (int p = 0; p < 2; ++p) newR += s.data[p] * R * s.data[p].adjoint();
            R = std::move(newR);
        }

        const SiteTensor& s = block.sites[siteIdx];
        double p0 = std::real((L * s.data[0] * R * s.data[0].adjoint()).trace());
        double p1 = std::real((L * s.data[1] * R * s.data[1].adjoint()).trace());
        outTotalNorm = p0 + p1;
        return p0;
    }

    int measure(int qubit, bool trace_out = false) {
        int n = (int)posToLoc.size();
        if (qubit < 0 || qubit >= n) throw std::out_of_range("Measure: qubit index out of range");
        trace_out = trace_out && (n > 1);

        Loc loc = posToLoc[qubit];
        Block& block = blocks.at(loc.blockId);
        int siteIdx = loc.localBit;

        double totalNorm = 1.0;
        double p0raw = computeP0(block, siteIdx, totalNorm);
        double p0 = (totalNorm > 1e-300) ? (p0raw / totalNorm) : 0.0;

        int outcome;
        if (randomMeasurements) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double rv = dist(rng);
            outcome = (rv < p0) ? 0 : 1;
        } else {
            outcome = 0;
        }

        double norm = std::sqrt((outcome == 0) ? p0 : (1.0 - p0));
        if (norm < TOLERANCE)
            throw std::runtime_error("Measurement probability ~0");

        SiteTensor& site = block.sites[siteIdx];
        if (trace_out) {
            MatrixC M = site.data[outcome] / norm; // becomes a plain bond matrix once its physical leg is projected out
            int nb = block.numQubits();
            if (nb == 1) {
                blocks.erase(loc.blockId);
            } else if (siteIdx + 1 < nb) {
                SiteTensor& rightSite = block.sites[siteIdx + 1];
                rightSite.data[0] = M * rightSite.data[0];
                rightSite.data[1] = M * rightSite.data[1];
                block.sites.erase(block.sites.begin() + siteIdx);
            } else {
                SiteTensor& leftSite = block.sites[siteIdx - 1];
                leftSite.data[0] = leftSite.data[0] * M;
                leftSite.data[1] = leftSite.data[1] * M;
                block.sites.erase(block.sites.begin() + siteIdx);
            }

            for (auto& l : posToLoc) {
                if (l.blockId == loc.blockId && l.localBit > siteIdx) l.localBit -= 1;
            }
            posToLoc.erase(posToLoc.begin() + qubit);
        } else {
            site.data[outcome] /= norm;
            site.data[1 - outcome].setZero();
        }

        return outcome;
    }

    int measure_qubit_in_basis(int qubit, MeasurementBasis basis, double alpha) {
        cplx psi0[2], psi1[2];
        switch (basis) {
            case MeasurementBasis::X:
                basis = MeasurementBasis::XY; break;
            case MeasurementBasis::Y:
                basis = MeasurementBasis::XY; alpha += M_PI/2; break;
            case MeasurementBasis::Z:
                basis = MeasurementBasis::XZ; break;
            default: break;
        }

        alpha = normalize_radians(alpha);
        switch (basis) {
            case MeasurementBasis::XY:
                psi0[0] = 1.0 / std::sqrt(2.0);
                psi0[1] = std::exp(cplx(0, alpha)) / std::sqrt(2.0);
                psi1[0] = 1.0 / std::sqrt(2.0);
                psi1[1] = -std::exp(cplx(0, alpha)) / std::sqrt(2.0);
                break;
            case MeasurementBasis::XZ:
                psi0[0] = std::cos(alpha/2.0);
                psi0[1] = std::sin(alpha/2.0);
                psi1[0] = std::sin(alpha/2.0);
                psi1[1] = -std::cos(alpha/2.0);
                break;
            case MeasurementBasis::YZ:
                psi0[0] = std::cos(alpha/2.0);
                psi0[1] = cplx(0,1) * std::sin(alpha/2.0);
                psi1[0] = std::sin(alpha/2.0);
                psi1[1] = -cplx(0,1) * std::cos(alpha/2.0);
                break;
            default:
                throw std::invalid_argument("Undefined measurement basis");
        }

        return measure_in_basis_vectors(qubit, psi0, psi1);
    }

    int measure_in_basis_vectors(int qubit, cplx psi0[2], cplx psi1[2]) {
        Matrix2C U;
        U(0,0) = psi0[0];
        U(1,0) = psi0[1];
        U(0,1) = psi1[0];
        U(1,1) = psi1[1];

        Matrix2C tmp = U.adjoint();
        U = tmp;

        apply_single_qubit_gate(qubit, U);
        return measure(qubit, true);
    }

    // ============== EQUALITY ==============
    static bool isEqual(const VectorC& a, const VectorC& b, double tolerance = TOLERANCE) {
        return vectorsEqual(a, b, tolerance);
    }

    static bool isEqualUpToGlobalPhase(const VectorC& a, const VectorC& b, double tolerance = TOLERANCE) {
        return vectorsEqualUpToGlobalPhase(a, b, tolerance);
    }
};

#endif // TENSOR_NETWORK_SIMULATOR_HPP
