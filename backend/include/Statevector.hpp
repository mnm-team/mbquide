#ifndef STATEVECTOR_SIMULATOR_HPP
#define STATEVECTOR_SIMULATOR_HPP

#include <Eigen/Dense>
#include <complex>
#include <iostream>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "utils.hpp"


class StatevectorSimulator {
public:
    using cplx = std::complex<double>;
    using VectorC = Eigen::VectorXcd;
    using Matrix2C = Eigen::Matrix2cd;

private:
    VectorC statevector;    // Eigen vector of complex amplitudes
    int num_qubits = 0;
    bool randomMeasurements = true;
    std::mt19937 rng;

    inline int get_bit(int number, int position) const {
        return (number >> position) & 1;
    }

    inline int set_bit(int number, int position, int value) const {
        if (value) return number | (1 << position);
        return number & ~(1 << position);
    }

    int remove_bit(int idx, int pos) const {
        int lower_mask = (1 << pos) - 1;
        int upper_mask = ~((1 << (pos + 1)) - 1);

        int lower = idx & lower_mask;
        int upper = (idx & upper_mask) >> 1;

        return lower | upper;
    }

    static cplx parseComplex(const std::string& s) {
        double real = 0.0, imag = 0.0;
        std::string trimmed = s;
        size_t start = trimmed.find_first_not_of(" 	");
        size_t end = trimmed.find_last_not_of(" 	");
        if (start != std::string::npos) trimmed = trimmed.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed == "0") return cplx(0.0, 0.0);

        size_t iPos = trimmed.find('i');
        if (iPos != std::string::npos) {
            size_t plusPos = trimmed.find(" + ");
            size_t minusPos = trimmed.find(" - ");
            if (plusPos != std::string::npos) {
                real = std::stod(trimmed.substr(0, plusPos));
                std::string imagStr = trimmed.substr(plusPos + 3, iPos - plusPos - 3);
                imag = std::stod(imagStr);
            } else if (minusPos != std::string::npos) {
                real = std::stod(trimmed.substr(0, minusPos));
                std::string imagStr = trimmed.substr(minusPos + 3, iPos - minusPos - 3);
                imag = -std::stod(imagStr);
            } else {
                std::string imagStr = trimmed.substr(0, iPos);
                imag = imagStr.empty() || imagStr == "+" ? 1.0 : imagStr == "-" ? -1.0 : std::stod(imagStr);
            }
        } else {
            real = std::stod(trimmed);
        }

        return cplx(real, imag);
    }

public:
    StatevectorSimulator() : StatevectorSimulator(0, true) {}
    StatevectorSimulator(int n, bool random = true)
        : num_qubits(n), randomMeasurements(random), rng(std::random_device{}()) {
        if (n < 0) throw std::invalid_argument("Number of qubits must be non-negative");
        int dim = (n == 0) ? 0 : (1 << n);
        if (dim > 0) {
            statevector = VectorC::Zero(dim);
            statevector(0) = cplx(1.0, 0.0);
        } else {
            statevector.resize(0);
        }
    }

    const VectorC& get_statevector() const { return statevector; }
    int get_num_qubits() const { return num_qubits; }

    void reset() {
        if (num_qubits == 0) {
            statevector.resize(0);
            return;
        }
        int dim = 1 << num_qubits;
        statevector = VectorC::Zero(dim);
        statevector(0) = cplx(1.0, 0.0);
    }

    std::string getStatevectorBraKet() const {
        auto fmt = [](cplx c) {
            std::ostringstream out;
            double r = c.real();
            double im = c.imag();
            if (std::abs(r) < TOLERANCE) r = 0;
            if (std::abs(im) < TOLERANCE) im = 0;
            if (r == 0 && im == 0) return std::string("0");
            out << "(";
            if (r != 0) out << r;
            if (im != 0) {
                if (im > 0 && r != 0) out << " + " << im << "i";
                else if (im < 0) out << " - " << std::abs(im) << "i";
                else if (r == 0) out << im << "i";
            }
            out << ")";
            return out.str();
        };

        size_t dim = statevector.size();
        size_t nq = 0;
        while ((1ULL << nq) < dim) nq++;
        std::ostringstream out;
        bool printedAnything = false;

        for (size_t i = 0; i < dim; ++i) {
            cplx amp = statevector(static_cast<int>(i));
            std::string ampStr = fmt(amp);
            if (ampStr == "0") continue;
            std::string ket;
            ket.reserve(nq);
            for (int q = (int)nq - 1; q >= 0; --q) ket.push_back(((i >> q) & 1) ? '1' : '0');
            if (!printedAnything) {
                out << ampStr << "|" << ket << ">";
                printedAnything = true;
            } else {
                out << " + " << ampStr << "|" << ket << ">";
            }
        }

        if (!printedAnything) out << "0";
        return out.str();
    }

    json toJson() const {
        json j = json::array();
        for (int i = 0; i < statevector.size(); ++i) {
            const cplx &c = statevector(i);
            j.push_back(json::array({c.real(), c.imag()}));
        }
        return j;
    }

    static VectorC parseBraKet(const std::string& braket) {
        std::vector<std::pair<cplx, size_t>> terms;
        size_t num_qubits_local = 0;
        size_t pos = 0;
        while (pos < braket.length()) {
            while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos;
            if (pos < braket.length() && braket[pos] == '+') { ++pos; while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos; }
            if (pos >= braket.length() || braket[pos] != '(') break;
            size_t start = pos + 1;
            size_t end = braket.find(')', start);
            if (end == std::string::npos) break;
            std::string ampStr = braket.substr(start, end - start);
            cplx amplitude = parseComplex(ampStr);
            pos = end + 1;
            while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos;
            if (pos >= braket.length() || braket[pos] != '|') break;
            ++pos;
            start = pos;
            end = braket.find('>', start);
            if (end == std::string::npos) break;
            std::string ketStr = braket.substr(start, end - start);
            num_qubits_local = std::max(num_qubits_local, ketStr.length());
            size_t index = 0;
            for (char c : ketStr) { index = (index << 1) | (c - '0'); }
            terms.push_back({amplitude, index});
            pos = end + 1;
        }

        if (terms.empty()) return VectorC();

        size_t dim = 1ULL << num_qubits_local;
        VectorC result = VectorC::Zero(dim);
        for (const auto& pr : terms) {
            if (pr.second < dim) result(static_cast<int>(pr.second)) = pr.first;
        }
        return result;
    }

    // setState: sets amplitudes for the entire system on a reset state |00..0>
    void setState(const VectorC& state) {
        int expected_state_size = 1 << num_qubits;
        if (state.size() != expected_state_size)
            throw std::invalid_argument("State vector size must be 2^num_qubits");

        // Must be in reset state
        if (num_qubits > 0) {
            if (std::abs(statevector(0) - cplx(1.0, 0.0)) > TOLERANCE)
                throw std::runtime_error("setState only works on reset statevector |00...0>");
            for (int i = 1; i < statevector.size(); ++i)
                if (std::abs(statevector(i)) > TOLERANCE)
                    throw std::runtime_error("setState only works on reset statevector |00...0>");
        }

        // Write amplitudes into statevector
        for (int i = 0; i < expected_state_size; ++i)
            statevector(i) = state(i);
    }

    // setStateSubsystem: sets amplitudes for some subsystem on a reset state |00..0>
    void setStateSubsystem(const std::vector<int>& qubits, const VectorC& state) {
        if (qubits.empty()) throw std::invalid_argument("Qubit list cannot be empty");
        int subregister_size = static_cast<int>(qubits.size());
        int expected_state_size = 1 << subregister_size;
        if (state.size() != expected_state_size) throw std::invalid_argument("State vector size must be 2^(number of qubits)");

        // Must be in reset state
        if (num_qubits > 0) {
            if (std::abs(statevector(0) - cplx(1.0, 0.0)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");
            for (int i = 1; i < statevector.size(); ++i) if (std::abs(statevector(i)) > TOLERANCE) throw std::runtime_error("setState only works on reset statevector |00...0>");
        }

        std::vector<int> sorted_qubits = qubits;
        std::sort(sorted_qubits.begin(), sorted_qubits.end());
        for (int i = 0; i < sorted_qubits.size(); ++i) {
            if (sorted_qubits[i] < 0 || sorted_qubits[i] >= num_qubits) throw std::out_of_range("Statevector setState: Qubit index out of range");
            if (i > 0 && sorted_qubits[i] == sorted_qubits[i-1]) throw std::invalid_argument("Duplicate qubit indices not allowed");
        }

        // write amplitudes into statevector
        for (int sub_idx = 0; sub_idx < expected_state_size; ++sub_idx) {
            int full_idx = 0;
            for (int i = 0; i < subregister_size; ++i) {
                int bit = (sub_idx >> i) & 1;
                full_idx = set_bit(full_idx, qubits[i], bit);
            }
            statevector(full_idx) = state(sub_idx);
        }
    }

    // Appends a new qubit in the |+> = (|0> + |1>) / sqrt(2) state.
    // The new qubit gets index num_qubits.
    // Implemented as a tensor product: |psi_new> = |psi_old> ⊗ |+>
    int add_qubit_plus() {
        int old_dim = (num_qubits == 0) ? 1 : (1 << num_qubits);
        int new_dim = old_dim << 1;
        VectorC new_state = VectorC::Zero(new_dim);
        const double inv_sqrt2 = 1.0 / std::sqrt(2.0);

        if (num_qubits == 0) {
            // no qubits yet, new state is just |+>
            new_state(0) = cplx(inv_sqrt2, 0.0);
            new_state(1) = cplx(inv_sqrt2, 0.0);
        } else {
            // |psi_old> ⊗ |+>:
            // old index i maps to:
            //   new index (i << 1) | 0  with amplitude * inv_sqrt2  (new qubit = |0>)
            //   new index (i << 1) | 1  with amplitude * inv_sqrt2  (new qubit = |1>)
            for (int i = 0; i < old_dim; ++i) {
                new_state(i << 1)       = statevector(i) * inv_sqrt2;
                new_state((i << 1) | 1) = statevector(i) * inv_sqrt2;
            }
        }

        num_qubits += 1;
        statevector = std::move(new_state);

        return num_qubits - 1;  // The ID of the new qubit
    }

    


    // ============== GATES ==============
    void apply_single_qubit_gate(int qubit, const Matrix2C& gate) {
        if (qubit < 0 || qubit >= num_qubits) throw std::out_of_range("Statevector SingleQgate: Qubit index out of range");
        int size = 1 << num_qubits;
        VectorC new_state = VectorC::Zero(size);
        for (int i = 0; i < size; ++i) {
            int bit = get_bit(i, qubit);
            int i_flipped = set_bit(i, qubit, 1 - bit);
            if (bit == 0) {
                new_state(i) = gate(0,0) * statevector(i) + gate(0,1) * statevector(i_flipped);
            } else {
                new_state(i) = gate(1,0) * statevector(i_flipped) + gate(1,1) * statevector(i);
            }
        }
        statevector = std::move(new_state);
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
        if (control < 0 || control >= num_qubits || target < 0 || target >= num_qubits) throw std::out_of_range("Statevector CZ: Qubit index out of range");
        if (control == target) throw std::invalid_argument("Control and target qubits must be different");

        int size = 1 << num_qubits;
        for (int i = 0; i < size; ++i) {
            int control_bit = get_bit(i, control);
            int target_bit = get_bit(i, target);
            if (control_bit == 1 && target_bit == 1) statevector(i) *= -1.0;
        }
    }

    // ============== MEASUREMENTS ==============
    int measure(int qubit, bool trace_out = false) {
        if (qubit < 0 || qubit >= num_qubits) throw std::out_of_range("Measure: qubit index out of range");
        int size = 1 << num_qubits;
        double p0 = 0.0;
        trace_out = trace_out && (num_qubits > 1);

        for (int i = 0; i < size; ++i) {
            int bit = get_bit(i, qubit);
            if (bit == 0) p0 += std::norm(statevector(i));
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
            int new_size = 1 << (num_qubits - 1);
            VectorC new_state = VectorC::Zero(new_size);
            for (int i = 0; i < size; ++i) {
                int bit = get_bit(i, qubit);
                if (bit == outcome) {
                    int new_idx = remove_bit(i, qubit);
                    new_state(new_idx) = statevector(i) / norm;
                }
            }
            statevector = std::move(new_state);
            num_qubits -= 1;
        } else {
            for (int i = 0; i < size; ++i) {
                int bit = get_bit(i, qubit);
                if (bit == outcome) statevector(i) /= norm;
                else statevector(i) = cplx(0.0, 0.0);
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
        // Build U such that U * [psi0 psi1] = identity basis -> we use rows = <0|, <1| applied to psi
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
        if (a.size() != b.size()) return false;

        for (int i = 0; i < a.size(); ++i) {
            if (std::abs(a(i) - b(i)) > tolerance)
                return false;
        }
        return true;
    }

    static bool isEqualUpToGlobalPhase(const VectorC& a, const VectorC& b, double tolerance = TOLERANCE) {
        if (a.size() != b.size()) return false;

        cplx phase_factor(1.0, 0.0);
        bool phase_found = false;

        for (int i = 0; i < a.size(); ++i) {
            cplx amp_a = a(i);
            cplx amp_b = b(i);

            if (!phase_found) {
                if (std::abs(amp_a) > tolerance && std::abs(amp_b) > tolerance) {
                    cplx ratio = amp_b / amp_a;
                    phase_factor = ratio / std::abs(ratio);
                    phase_found = true;
                } else if (std::abs(amp_a) > tolerance || std::abs(amp_b) > tolerance) {
                    return false;
                }
                continue;
            }

            if (std::abs(amp_a * phase_factor - amp_b) > tolerance)
                return false;
        }

        return true;
    }
    
};

#endif // STATEVECTOR_SIMULATOR_HPP
