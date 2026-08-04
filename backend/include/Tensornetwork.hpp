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
// a qubit out shifts everything above it down by one

class TensorNetworkSimulator {
public:
    using cplx = std::complex<double>;
    using VectorC = Eigen::VectorXcd;
    using Matrix2C = Eigen::Matrix2cd;

private:
    struct Block {
        VectorC amp;        // 2^numQubits amplitudes
        int numQubits = 0;
    };

    // Which block a given global qubit position currently lives in, and at
    // which local bit position inside that block's amplitude vector.
    struct Loc {
        int blockId;
        int localBit;
    };

    std::unordered_map<int, Block> blocks;
    int nextBlockId = 0;
    std::vector<Loc> posToLoc;   // posToLoc[globalPos] -> location

    bool randomMeasurements = true;
    std::mt19937 rng;

    int newBlockId() { return nextBlockId++; }

    static void apply_gate_to_block(Block& block, int bit, const Matrix2C& gate) {
        int size = 1 << block.numQubits;
        VectorC new_state = VectorC::Zero(size);
        for (int i = 0; i < size; ++i) {
            int b = get_bit(i, bit);
            int i_flipped = set_bit(i, bit, 1 - b);
            if (b == 0) new_state(i) = gate(0,0) * block.amp(i) + gate(0,1) * block.amp(i_flipped);
            else        new_state(i) = gate(1,0) * block.amp(i_flipped) + gate(1,1) * block.amp(i);
        }
        block.amp = std::move(new_state);
    }

    struct MergedRef { int blockId; int localU; int localV; };

    // Ensures the qubits at global positions u and v share a single block
    // (merging their two blocks via a tensor/kron product if they don't
    // already), and returns that block together with u and v's local bit
    // positions inside it.
    MergedRef ensureSameBlock(int posU, int posV) {
        Loc lu = posToLoc[posU];
        Loc lv = posToLoc[posV];
        if (lu.blockId == lv.blockId) {
            return {lu.blockId, lu.localBit, lv.localBit};
        }

        Block& A = blocks.at(lu.blockId);
        Block& B = blocks.at(lv.blockId);
        int nA = A.numQubits;
        int dimA = 1 << nA;
        int dimB = 1 << B.numQubits;

        Block merged;
        merged.numQubits = nA + B.numQubits;
        merged.amp = VectorC::Zero(1 << merged.numQubits);
        // bits [0, nA) come from A, bits [nA, nA+nB) come from B.
        for (int ia = 0; ia < dimA; ++ia) {
            cplx ca = A.amp(ia);
            if (ca == cplx(0.0, 0.0)) continue;
            for (int ib = 0; ib < dimB; ++ib) {
                merged.amp(ia | (ib << nA)) = ca * B.amp(ib);
            }
        }

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
            std::vector<int> positions(block.numQubits, -1);
            for (int p = 0; p < n; ++p) {
                if (posToLoc[p].blockId == id) positions[posToLoc[p].localBit] = p;
            }

            int dimOld = (int)temp.size();
            int dimBlock = 1 << block.numQubits;
            VectorC next = VectorC::Zero(dimOld * dimBlock);
            for (int i = 0; i < dimOld; ++i) {
                cplx c = temp(i);
                if (c == cplx(0.0, 0.0)) continue;
                for (int j = 0; j < dimBlock; ++j) {
                    next(i | (j << bitsSoFar)) = c * block.amp(j);
                }
            }
            temp = std::move(next);

            for (int localBit = 0; localBit < block.numQubits; ++localBit) {
                tempBitOfGlobalPos[positions[localBit]] = bitsSoFar + localBit;
            }
            bitsSoFar += block.numQubits;
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
    TensorNetworkSimulator(int n, bool random = true)
        : randomMeasurements(random), rng(std::random_device{}()) {
        if (n < 0) throw std::invalid_argument("Number of qubits must be non-negative");
        if (n > 0) {
            Block b;
            b.numQubits = n;
            b.amp = VectorC::Zero(1 << n);
            b.amp(0) = cplx(1.0, 0.0);
            int id = newBlockId();
            blocks.emplace(id, std::move(b));
            posToLoc.resize(n);
            for (int i = 0; i < n; ++i) posToLoc[i] = Loc{id, i};
        }
    }

    int get_num_qubits() const { return (int)posToLoc.size(); }

    VectorC get_statevector() const { return contractAll(); }

    void reset() {
        int n = (int)posToLoc.size();
        blocks.clear();
        posToLoc.clear();
        if (n > 0) {
            Block b;
            b.numQubits = n;
            b.amp = VectorC::Zero(1 << n);
            b.amp(0) = cplx(1.0, 0.0);
            int id = newBlockId();
            blocks.emplace(id, std::move(b));
            posToLoc.resize(n);
            for (int i = 0; i < n; ++i) posToLoc[i] = Loc{id, i};
        }
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
    // all qubits still form a single fresh block (the |00...0> state).
    void setState(const VectorC& state) {
        int n = (int)posToLoc.size();
        int expected_state_size = 1 << n;
        if (state.size() != expected_state_size)
            throw std::invalid_argument("State vector size must be 2^num_qubits");
        if (n == 0) return;

        int blockId = posToLoc[0].blockId;
        Block& block = blocks.at(blockId);
        if (block.numQubits != n)
            throw std::runtime_error("setState only works on reset statevector |00...0>");

        if (std::abs(block.amp(0) - cplx(1.0, 0.0)) > TOLERANCE)
            throw std::runtime_error("setState only works on reset statevector |00...0>");
        for (int i = 1; i < block.amp.size(); ++i)
            if (std::abs(block.amp(i)) > TOLERANCE)
                throw std::runtime_error("setState only works on reset statevector |00...0>");

        for (int i = 0; i < expected_state_size; ++i)
            block.amp(i) = state(i);
    }

    void setStateSubsystem(const std::vector<int>& qubits, const VectorC& state) {
        if (qubits.empty()) throw std::invalid_argument("Qubit list cannot be empty");
        int subregister_size = static_cast<int>(qubits.size());
        int expected_state_size = 1 << subregister_size;
        if (state.size() != expected_state_size) throw std::invalid_argument("State vector size must be 2^(number of qubits)");

        int n = (int)posToLoc.size();
        if (n == 0) throw std::out_of_range("setStateSubsystem: no qubits present");
        int blockId = posToLoc[0].blockId;
        Block& block = blocks.at(blockId);
        if (block.numQubits != n)
            throw std::runtime_error("setState only works on reset statevector |00...0>");

        if (std::abs(block.amp(0) - cplx(1.0, 0.0)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");
        for (int i = 1; i < block.amp.size(); ++i) if (std::abs(block.amp(i)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");

        std::vector<int> sorted_qubits = qubits;
        std::sort(sorted_qubits.begin(), sorted_qubits.end());
        for (size_t i = 0; i < sorted_qubits.size(); ++i) {
            if (sorted_qubits[i] < 0 || sorted_qubits[i] >= n) throw std::out_of_range("TensorNetwork setState: Qubit index out of range");
            if (i > 0 && sorted_qubits[i] == sorted_qubits[i-1]) throw std::invalid_argument("Duplicate qubit indices not allowed");
        }

        for (int sub_idx = 0; sub_idx < expected_state_size; ++sub_idx) {
            int full_idx = 0;
            for (int i = 0; i < subregister_size; ++i) {
                int bit = (sub_idx >> i) & 1;
                full_idx = set_bit(full_idx, qubits[i], bit);
            }
            block.amp(full_idx) = state(sub_idx);
        }
    }

    // New qubit takes position 0; every previously active qubit's position shifts up by one
    int add_qubit_plus() {
        Block b;
        b.numQubits = 1;
        const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
        b.amp = VectorC(2);
        b.amp(0) = cplx(inv_sqrt2, 0.0);
        b.amp(1) = cplx(inv_sqrt2, 0.0);

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
        apply_gate_to_block(blocks.at(loc.blockId), loc.localBit, gate);
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
        int size = 1 << block.numQubits;
        for (int i = 0; i < size; ++i) {
            if (get_bit(i, ref.localU) == 1 && get_bit(i, ref.localV) == 1) block.amp(i) *= -1.0;
        }
    }

    // ============== MEASUREMENTS ==============
    int measure(int qubit, bool trace_out = false) {
        int n = (int)posToLoc.size();
        if (qubit < 0 || qubit >= n) throw std::out_of_range("Measure: qubit index out of range");
        trace_out = trace_out && (n > 1);

        Loc loc = posToLoc[qubit];
        Block& block = blocks.at(loc.blockId);
        int bit = loc.localBit;
        int size = 1 << block.numQubits;

        double p0 = 0.0;
        for (int i = 0; i < size; ++i) {
            if (get_bit(i, bit) == 0) p0 += std::norm(block.amp(i));
        }

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

        if (trace_out) {
            int new_size = size >> 1;
            VectorC new_amp = VectorC::Zero(new_size);
            for (int i = 0; i < size; ++i) {
                if (get_bit(i, bit) == outcome) {
                    int new_idx = remove_bit(i, bit);
                    new_amp(new_idx) = block.amp(i) / norm;
                }
            }
            block.amp = std::move(new_amp);
            block.numQubits -= 1;

            int blockId = loc.blockId;
            if (block.numQubits == 0) {
                blocks.erase(blockId);
            } else {
                for (auto& l : posToLoc) {
                    if (l.blockId == blockId && l.localBit > bit) l.localBit -= 1;
                }
            }
            posToLoc.erase(posToLoc.begin() + qubit);
        } else {
            for (int i = 0; i < size; ++i) {
                if (get_bit(i, bit) == outcome) block.amp(i) /= norm;
                else block.amp(i) = cplx(0.0, 0.0);
            }
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
