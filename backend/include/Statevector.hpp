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
#include "QuantumVector.hpp"


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
        return vectorToBraKet(statevector);
    }

    json toJson() const {
        return vectorToJson(statevector);
    }

    static VectorC parseBraKet(const std::string& braket) {
        return parseBraKetVector(braket);
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


    // Reorders qubits according to a permutation.
    // permutation[new_qubit_index] = old_qubit_index
    // e.g. permutation = {2, 0, 1} means: new bit 0 = old bit 2, etc.
    void reorderQubits(const std::vector<int>& permutation) {
        if ((int)permutation.size() != num_qubits)
            throw std::invalid_argument("reorderQubits: permutation size mismatch");

        int dim = 1 << num_qubits;
        VectorC new_state = VectorC::Zero(dim);

        for (int old_idx = 0; old_idx < dim; ++old_idx) {
            int new_idx = 0;
            for (int new_bit = 0; new_bit < num_qubits; ++new_bit) {
                int old_bit = permutation[new_bit];
                int bit_val = get_bit(old_idx, old_bit);
                new_idx = set_bit(new_idx, new_bit, bit_val);
            }
            new_state(new_idx) = statevector(old_idx);
        }

        statevector = std::move(new_state);
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
        return vectorsEqual(a, b, tolerance);
    }

    static bool isEqualUpToGlobalPhase(const VectorC& a, const VectorC& b, double tolerance = TOLERANCE) {
        return vectorsEqualUpToGlobalPhase(a, b, tolerance);
    }

};

#endif // STATEVECTOR_SIMULATOR_HPP
