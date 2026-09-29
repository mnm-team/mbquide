#pragma once

#include "ZX_Graph.hpp"
#include <filesystem>
#include <fstream>
#include <filesystem>
#include <string>
#include <optional>
#include "MBQC_Graph.hpp"
#include "Flow.hpp"

/**
 * @brief Test-only helper: checks whether two ZX diagrams represent the same
 * linear map (up to global scalar), by exporting both to PyZX JSON and
 * shelling out to `backend/test/compare_tensors.py` (which uses `pyzx`'s
 * tensor contraction to compare them).
 * @return Whether the Python comparison script exited successfully (`true`) indicating the tensors match.
 */
inline bool compareTensors(ZXGraph zx1, ZXGraph zx2) {
    std::string file1 = "ComparisonZX_1.json";
    std::string file2 = "ComparisonZX_2.json";

    std::ofstream(file1) << zx1.toPyZXJson().dump(4);
    std::ofstream(file2) << zx2.toPyZXJson().dump(4);

    std::string pythonCommand = "python_venv/bin/python backend/test/compare_tensors.py " + file1 + " " + file2;
    int result = system(pythonCommand.c_str());

    std::filesystem::remove(file1);
    std::filesystem::remove(file2);

    return result == 0;
}



/**
 * @brief Test-only helper: generates a random Clifford(+T) circuit as an
 * OpenQASM string by shelling out to `backend/test/random_clifford.py`.
 * @param num_qubits Number of qubits in the generated circuit.
 * @param depth Number of gates to generate.
 * @param p_t,p_s,p_hsh,p_cnot Optional relative weights for how often T, S,
 * H-S-H, and CNOT gates (respectively) are chosen; passed through to the
 * Python script's `--p_*` flags when given.
 * @return The generated QASM source, or `""` if the Python script couldn't be run or failed.
 */
inline std::string randomClifford(
    int num_qubits,
    int depth,
    std::optional<double> p_t = std::nullopt,
    std::optional<double> p_s = std::nullopt,
    std::optional<double> p_hsh = std::nullopt,
    std::optional<double> p_cnot = std::nullopt
) {
    std::string command =
        "python_venv/bin/python backend/test/random_clifford.py " +
        std::to_string(num_qubits) + " " +
        std::to_string(depth);

    if (p_t)    command += " --p_t " + std::to_string(*p_t);
    if (p_s)    command += " --p_s " + std::to_string(*p_s);
    if (p_hsh)  command += " --p_hsh " + std::to_string(*p_hsh);
    if (p_cnot) command += " --p_cnot " + std::to_string(*p_cnot);

    std::array<char, 256> buffer{};
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: popen() failed\n";
        return "";  // graceful fallback
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();

    int rc = pclose(pipe);
    if (rc != 0) {
        std::cerr << "Error: Python script failed with code " << rc << "\n";
        return "";  // or return partial result if you prefer
    }

    return result;
}


/**
 * @brief Test-only helper: independently verifies that `flow` satisfies the
 * formal definition of a Pauli flow for `g` (as opposed to trusting
 * findPauliFlow()'s own construction), by checking, for every non-output
 * vertex `u`, the ordering/self-membership conditions on its correction set
 * and odd neighborhood implied by `u`'s measurement basis.
 * @return `true` if `flow` is a valid Pauli flow for `g` (also `false` if `flow.ok` is already `false`); `false` (with a diagnostic to stdout) on the first violation found.
 */
inline bool checkPauliFlow(
    const MBQC_Graph& g,
    const PauliFlowResult& flow
) {

    if (!flow.ok) return false;

    for (int u : g.getNonOutputs()) {

        const auto& corrections = flow.corrf.at(u);
        auto odd_nh = g.oddNeighborhood(corrections);

        int u_order = flow.depths.at(u);

        // <X
        for (int v : corrections) {

            auto basis_v = g.getMeasurement(v).first;

            if (
                v != u &&
                basis_v != MeasurementBasis::X &&
                basis_v != MeasurementBasis::Y &&
                flow.depths.at(u) <= flow.depths.at(v)
            ) {
                std::cout
                    << u << " is measured after " << v
                    << ", but " << v
                    << " is not X or Y measured and occurs in the correction set of "
                    << u << "\n";

                return false;
            }
        }

        // <Z
        for (int v : odd_nh) {

            auto basis_v = g.getMeasurement(v).first;

            if (
                v != u &&
                basis_v != MeasurementBasis::Y &&
                basis_v != MeasurementBasis::Z &&
                flow.depths.at(u) <= flow.depths.at(v)
            ) {
                std::cout
                    << u << " is measured after " << v
                    << ", but " << v
                    << " is not Y or Z measured and occurs in the odd neighborhood of the correction set of "
                    << u << "\n";

                return false;
            }
        }

        // <Y
        std::unordered_set<int> ys_1;
        std::unordered_set<int> ys_2;

        for (int v : corrections) {

            auto basis_v = g.getMeasurement(v).first;

            if (
                basis_v == MeasurementBasis::Y &&
                flow.depths.at(v) >= u_order &&
                v != u
            ) {
                ys_1.insert(v);
            }
        }

        for (int v : odd_nh) {

            auto basis_v = g.getMeasurement(v).first;

            if (
                basis_v == MeasurementBasis::Y &&
                flow.depths.at(v) >= u_order &&
                v != u
            ) {
                ys_2.insert(v);
            }
        }

        if (ys_1 != ys_2) {

            std::cout
                << "A Y-measured vertex in the correction set of "
                << u
                << " does not occur in the odd neighborhood of the correction set or vice versa.\n";

            return false;
        }

        auto basis_u = g.getMeasurement(u).first;

        // lXY, XZ, YZ
        if (basis_u == MeasurementBasis::XY) {

            if (
                corrections.count(u) ||
                !odd_nh.count(u)
            ) {
                std::cout
                    << u
                    << " is XY-measured but either occurs in its correction set or does not occur in the odd neighborhood of its correction set.\n";

                return false;
            }

        } else if (basis_u == MeasurementBasis::XZ) {

            if (
                !corrections.count(u) ||
                !odd_nh.count(u)
            ) {
                std::cout
                    << u
                    << " is XZ-measured but either does not occur in its correction set or does not occur in the odd neighborhood of its correction set.\n";

                return false;
            }

        } else if (basis_u == MeasurementBasis::YZ) {

            if (
                !corrections.count(u) ||
                odd_nh.count(u)
            ) {
                std::cout
                    << u
                    << " is YZ-measured but either does not occur in its correction set or does occur in the odd neighborhood of its correction set.\n";

                return false;
            }

        // lX,Z,Y
        } else if (basis_u == MeasurementBasis::X) {

            if (!odd_nh.count(u)) {

                std::cout
                    << u
                    << " is X-measured but does not occur in the odd neighborhood of its correction set.\n";

                return false;
            }

        } else if (basis_u == MeasurementBasis::Z) {

            if (!corrections.count(u)) {

                std::cout
                    << u
                    << " is Z-measured but does not occur in its correction set.\n";

                return false;
            }

        } else if (basis_u == MeasurementBasis::Y) {

            bool in_corr = corrections.count(u);
            bool in_odd  = odd_nh.count(u);

            if (!(in_corr ^ in_odd)) {

                std::cout
                    << u
                    << " is Y-measured but is not exclusively in either its correction set or the odd neighborhood of its correction set.\n";

                return false;
            }

        } else {

            std::cout
                << "Error: "
                << u
                << " has no measurement effect.\n";

            return false;
        }
    }

    return true;
}


/**
 * @brief Test-only helper: independently verifies that `flow` is *focused*
 * (as opposed to trusting focus()'s own construction) — that every
 * correction-set/odd-neighborhood entry it references (excluding outputs and
 * self-references) is measured in a basis compatible with still being a
 * pending correction target.
 * @return `true` if `flow` is a valid focused flow for `g` (also `false` if `flow.ok` is already `false`); `false` (with a diagnostic to stdout) on the first violation found.
 */
inline bool checkFocussedFlow(
    const MBQC_Graph& g,
    const PauliFlowResult& flow
) {

    if (!flow.ok) {
        std::cout << "Flow is not valid.\n";
        return false;
    }

    auto outputsVec = g.getOutputs();
    std::unordered_set<int> outputs(
        outputsVec.begin(),
        outputsVec.end()
    );
    

    for (int v : g.getNonOutputs()) {

        // correction set
        std::unordered_set<int> check_corrections;

        for (int w : flow.corrf.at(v)) {
            if (!outputs.count(w) && w != v) {
                check_corrections.insert(w);
            }
        }

        // odd neighborhood
        auto odd_full = g.oddNeighborhood(flow.corrf.at(v));

        std::unordered_set<int> check_odd_nh;

        for (int w : odd_full) {
            if (!outputs.count(w) && w != v) {
                check_odd_nh.insert(w);
            }
        }

        // FX
        for (int w : check_corrections) {

            auto basis = g.getMeasurement(w).first;

            bool valid =
                basis == MeasurementBasis::XY ||
                basis == MeasurementBasis::X  ||
                basis == MeasurementBasis::Y;

            if (!valid) {

                std::cout
                    << "A vertex in the correction set of "
                    << v
                    << " is not measured in XY, X or Y plane\n";

                return false;
            }
        }

        // FZ
        for (int w : check_odd_nh) {

            auto basis = g.getMeasurement(w).first;

            bool valid =
                basis == MeasurementBasis::XZ ||
                basis == MeasurementBasis::YZ ||
                basis == MeasurementBasis::Y  ||
                basis == MeasurementBasis::Z;

            if (!valid) {

                std::cout
                    << "A vertex in the odd neighborhood of the correction set of "
                    << v
                    << " is not measured in XZ, YZ, Y or Z plane\n";

                return false;
            }
        }

        // FY
        std::unordered_set<int> ys_corr;
        std::unordered_set<int> ys_odd;

        for (int y : check_corrections) {
            if (g.getMeasurement(y).first == MeasurementBasis::Y) {
                ys_corr.insert(y);
            }
        }

        for (int y : check_odd_nh) {
            if (g.getMeasurement(y).first == MeasurementBasis::Y) {
                ys_odd.insert(y);
            }
        }

        if (ys_corr != ys_odd) {

            std::cout
                << "A Y-measured vertex in the correction set of "
                << v
                << " does not occur in the odd neighborhood of the correction set or vice versa\n";

            return false;
        }
    }

    return true;
}