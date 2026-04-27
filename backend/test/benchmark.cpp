#include "doctest.h"
#include "utils.hpp"
#include "test_helpers.hpp"
#include "Simulator.hpp"
#include "Statevector.hpp"
#include "MBQC_Graph.hpp"
#include "Flow.hpp"
#include "ZX_Graph.hpp"
#include "ZX2MBQC.hpp"
#include "QASM_Parser.hpp"
#include "Quantum_Circuit.hpp"

#include <cstddef>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <optional>
#include <vector>
#include <string>
#include <numeric>
#include <cmath>


// =============================================
// Timing Utilities
// =============================================

using Clock = std::chrono::high_resolution_clock;
using Micros = std::chrono::microseconds;

struct StageTiming {
    double parse_us       = 0;
    double zx_build_us    = 0;
    double zx2mbqc_us     = 0;
    double flow_us        = 0;
    double simulate_us    = 0;
    double total_us       = 0;
};

struct GraphStats {
    float mbqc_nodes_before  = 0;
    float mbqc_edges_before  = 0;
    float mbqc_nodes_after   = 0;
    float mbqc_edges_after   = 0;
    bool flow_found        = false;
};

// Run the full pipeline and collect per-stage timings and graph stats.
// simplify=true applies MBQC_Graph::simplify() before flow-finding.
StageTiming benchmarkPipeline(
    const std::string& qasmText,
    const std::string& inputState,
    GraphStats& stats,
    bool simplify = false,
    int repetitions = 1,
    bool conveyorBelt = true)
{
    StageTiming acc;

    for (int r = 0; r < repetitions; ++r) {

        // ----- Stage 1: QASM parse -----
        auto t0 = Clock::now();
        QASMParser parser("", qasmText);
        QuantumCircuit circ = parser.parse();
        auto t1 = Clock::now();

        // ----- Stage 2: ZX graph construction -----
        ZXGraph zx = ZXGraph::fromQuantumCircuit(circ);
        auto t2 = Clock::now();

        // ----- Stage 3: ZX → MBQC translation -----
        MBQC_Graph graph = ZXtoMBQCGraph(zx);
        auto t3 = Clock::now();

        // Capture pre-simplify stats on first repetition
        if (r == 0) {
            stats.mbqc_nodes_before = graph.getSize();
            stats.mbqc_edges_before = (int)graph.getAllEdges().size() / 2; // symmetric
        }

        // Optional simplification (not timed as a separate stage here,
        // but you can split it out if needed)
        if (simplify) {
            graph.simplify();
        }

        if (r == 0) {
            stats.mbqc_nodes_after = graph.getSize();
            stats.mbqc_edges_after = (int)graph.getAllEdges().size() / 2;
        }

        // ----- Stage 4: Flow finding -----
        auto t4 = Clock::now();
        PauliFlowResult flow = findPauliFlow(graph);
        auto t5 = Clock::now();

        if (r == 0) stats.flow_found = flow.ok;

        // ----- Stage 5: Simulation -----
        auto t6 = Clock::now();
        if (flow.ok) {
            Simulator sim(graph, flow, true, inputState, 128, conveyorBelt);
            sim.simulateAll();
        }
        auto t7 = Clock::now();

        // Accumulate
        acc.parse_us    += std::chrono::duration_cast<Micros>(t1 - t0).count();
        acc.zx_build_us += std::chrono::duration_cast<Micros>(t2 - t1).count();
        acc.zx2mbqc_us  += std::chrono::duration_cast<Micros>(t3 - t2).count();
        acc.flow_us     += std::chrono::duration_cast<Micros>(t5 - t4).count();
        acc.simulate_us += std::chrono::duration_cast<Micros>(t7 - t6).count();
        acc.total_us    += std::chrono::duration_cast<Micros>(t7 - t0).count();
    }

    // Average over repetitions
    acc.parse_us    /= repetitions;
    acc.zx_build_us /= repetitions;
    acc.zx2mbqc_us  /= repetitions;
    acc.flow_us     /= repetitions;
    acc.simulate_us /= repetitions;
    acc.total_us    /= repetitions;

    return acc;
}



// =============================================
// BENCHMARK: Conveyor Belt vs Standard
// =============================================

TEST_CASE("Benchmark: Random Clifford - Conveyor Belt Comparison") {

    const int REPS = 10;

    auto makeZeroInput = [](int n) -> std::string {
        return "(1)|" + std::string(n, '0') + ">";
    };

    std::vector<int> qubitSizes = {5};
    int max_depth = 150;
    int depth_step = 5;

    for (int nq : qubitSizes) {

        std::cout << "\n============================================================\n";
        std::cout << " Random Clifford comparison — " << nq << " qubits\n";
        std::cout << "============================================================\n\n";

        std::cout << std::left  << std::setw(18) << "Depth"

                  << std::setw(45) << "Standard pipeline"
                  << std::setw(45) << "Conveyor belt pipeline"
                  << "\n";

        std::cout << std::left << std::setw(18) << " "
                  << std::right
                  << std::setw(10) << "Total µs"
                  << std::setw(10) << "Nodes"
                  << std::setw(10) << "Reduce%"
                  << "   |   "
                  << std::setw(10) << "Total µs"
                  << std::setw(10) << "Nodes"
                  << std::setw(10) << "Reduce%"
                  << "\n";

        std::cout << std::string(100, '-') << "\n";

        for (int depth = depth_step; depth <= max_depth; depth += depth_step) {

            std::string input = makeZeroInput(nq);

            StageTiming accStd, accConv;
            GraphStats  statsStd, statsConv;

            for (int rep = 0; rep < REPS; ++rep) {
                std::string qasm = randomClifford(nq, depth, std::nullopt, std::nullopt, std::nullopt, 0.2);

                GraphStats  gsStd, gsConv;
                StageTiming tStd  = benchmarkPipeline(qasm, input, gsStd,  true, 1, false);
                StageTiming tConv = benchmarkPipeline(qasm, input, gsConv, true, 1, true);

                // Accumulate
                accStd.simulate_us  += tStd.simulate_us;
                accConv.simulate_us += tConv.simulate_us;

                statsStd.mbqc_nodes_after  += gsStd.mbqc_nodes_after;
                statsConv.mbqc_nodes_after += gsConv.mbqc_nodes_after;
                statsStd.mbqc_nodes_before  += gsStd.mbqc_nodes_before;
                statsConv.mbqc_nodes_before += gsConv.mbqc_nodes_before;
            }

            // Average
            accStd.simulate_us  /= REPS;
            accConv.simulate_us /= REPS;

            statsStd.mbqc_nodes_after  /= REPS;
            statsConv.mbqc_nodes_after /= REPS;
            statsStd.mbqc_nodes_before  /= REPS;
            statsConv.mbqc_nodes_before /= REPS;

            double redStd = (statsStd.mbqc_nodes_before > 0)
                ? 100.0 * (1.0 - (double)statsStd.mbqc_nodes_after / statsStd.mbqc_nodes_before)
                : 0.0;
            double redConv = (statsConv.mbqc_nodes_before > 0)
                ? 100.0 * (1.0 - (double)statsConv.mbqc_nodes_after / statsConv.mbqc_nodes_before)
                : 0.0;


            std::cout << std::left  << std::setw(18) << depth
                      << std::right << std::fixed << std::setprecision(1)

                      // Standard
                      << std::setw(10) << accStd.simulate_us
                      << std::setw(10) << statsStd.mbqc_nodes_after
                      << std::setw(10) << redStd
                      << "   |   "

                      // Conveyor
                      << std::setw(10) << accConv.simulate_us
                      << std::setw(10) << statsConv.mbqc_nodes_after
                      << std::setw(10) << redConv
                      << "\n";
        }
    }

    CHECK(true);
}

