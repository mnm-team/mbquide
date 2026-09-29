#ifndef OUTPUTADJUSTMENT_HPP
#define OUTPUTADJUSTMENT_HPP

#include <Eigen/Dense>
#include <complex>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Quantum_Circuit.hpp"

const std::complex<double> J(0,1);

/**
 * @brief Tracks the accumulated Pauli-frame correction on an MBQC output
 * qubit as a 2x2-matrix stabilizer tableau, analogous to Stim's Pauli-frame
 * tracking (https://arxiv.org/pdf/2103.02202, section 2.3).
 *
 * Rather than applying each X/Z correction gate to the output qubit's actual
 * quantum state immediately, MBQC_Graph and Simulator instead conjugate the
 * X and Z Pauli operators through every correction as it's discovered
 * (adjustOutput()), and only realize the net effect as real gates once the
 * output is finally read out (toCircuit(), used by Simulator::step()). This
 * defers work and keeps corrections symbolic until they're needed.
 */
class OutputAdjustmentMap {
private:
    std::map<std::string, Eigen::Matrix2cd> adjustments_;  // {"X": Matrix2cd, "Z": Matrix2cd}

    /// Maps an operation name ("x", "z", "h", "rx", "rz", "s", "-s", case-insensitive) plus angle (for "rx"/"rz") to its 2x2 unitary matrix.
    static Eigen::Matrix2cd string2OperationMatrix(const std::string& operationString, double angle);
    /// Conjugates `P` by `U`: returns `U * P * U†`.
    static Eigen::Matrix2cd conjugate(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P);
    /// Conjugates `P` by `U`'s inverse: returns `U† * P * U`.
    static Eigen::Matrix2cd conjugate_other(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P);
    /// Identifies which (signed) single-qubit Pauli (`"X"`,`"Y"`,`"Z"`,`"I"`, sign `+1`/`-1`) a 2x2 matrix equals, or `{"Unknown", 0}` if it isn't one.
    static std::pair<std::string, int> identifyPauli(const Eigen::Matrix2cd& P);
    /// Inverse of identifyPauli(): builds the signed Pauli matrix for a `{label, sign}` pair.
    static Eigen::Matrix2cd parsePauli(const std::pair<std::string, int> pauliPair);

public:
    /// Constructs a standard (no-op) adjustment: X -> X, Z -> Z. Equivalent to calling reset().
    OutputAdjustmentMap();

    /**
     * @brief Conjugates the tracked X and Z operators by the named single-qubit
     * operation, recording that a correction/gate was applied to this output.
     * @param operationString One of `"X"`, `"Z"`, `"H"`, `"Rx"`, `"Rz"`, `"S"`, `"-S"` (case-insensitive).
     * @param angle Rotation angle, used only by `"Rx"`/`"Rz"`.
     */
    void adjustOutput(const std::string& operationString, double angle = 0);

    /// Human-readable summary, e.g. `"X: +X, Z: -Z"`.
    std::string toString() const;
    /// Serializes the tracked X/Z operators as `{"X": [pauli, sign], "Z": [pauli, sign]}`.
    json toJson() const;
    /// Inverse of toJson().
    static OutputAdjustmentMap fromJson(const json& j);

    /**
     * @brief Extracts a minimal single-qubit QuantumCircuit of {H, S, Z} gates
     * that realizes the currently tracked adjustment when applied to the
     * output qubit, and would reset the tracked X/Z operators back to
     * standard (X, Z) if conjugated through it in reverse.
     *
     * Based on the tableau-to-circuit synthesis in
     * https://github.com/quantumlib/Stim/blob/197d9a64ea0734cf83a610601223f34fc6065ada/src/stim/util_top/circuit_vs_tableau.inl#L52
     */
    QuantumCircuit toCircuit();

    /// Resets the tracked adjustment back to standard: X -> X, Z -> Z.
    void reset();

    /// Direct access to the internal `{"X": ..., "Z": ...}` operator map.
    std::map<std::string, Eigen::Matrix2cd>& getMap() { return adjustments_; }
    /// @overload
    const std::map<std::string, Eigen::Matrix2cd>& getMap() const { return adjustments_; }

    /// Whether no correction is currently pending (X -> X and Z -> Z, unchanged since construction/reset()).
    bool isStandard() const;
};

#endif // OUTPUTADJUSTMENT_HPP
