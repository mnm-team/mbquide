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
#include <fstream>
#include <filesystem>


// =============================================
// CSV output helpers
// =============================================

// Opens backend/test/results/<filename> for writing (creating the directory
// if needed) and writes the given header as its first line. Run the
// benchmark binary from the repository root (same requirement as
// randomClifford()) so this relative path resolves correctly.
inline std::ofstream openResultsCsv(const std::string& filename, const std::string& header) {
    std::filesystem::path dir = "backend/test/results";
    std::filesystem::create_directories(dir);
    std::ofstream out(dir / filename);
    out << header << "\n";
    return out;
}


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
    bool conveyorBelt = true,
    std::string backend = "statevector")
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
            Simulator sim(graph, flow, true, inputState, 128, conveyorBelt, backend);
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

    // Matches the methodology described in the paper's Evaluation section
    // (Figure 8): a single 5-qubit circuit family, circuit depth 5 to 150
    // in steps of 5, averaged over 10 independently sampled circuits per
    // depth, on the dense (state vector) backend. "Standard" = full
    // initialization (conveyorBelt=false, every qubit allocated up front);
    // "Conveyor belt" = the dynamic/partial initialization strategy of
    // Section IV-E2 (conveyorBelt=true).
    const int REPS = 10;
    const int nq = 5;
    const int max_depth = 150;
    const int depth_step = 5;

    auto makeZeroInput = [](int n) -> std::string {
        return "(1)|" + std::string(n, '0') + ">";
    };

    std::ofstream csv = openResultsCsv(
        "full_vs_partial.csv",
        "depth,rep,nodes_after,full_us,full_ok,partial_us,partial_ok");

    std::cout << "\n============================================================\n";
    std::cout << " Conveyor Belt comparison -- " << nq << " qubits\n";
    std::cout << "============================================================\n\n";

    std::cout << std::left  << std::setw(18) << "Depth"
              << std::setw(45) << "Standard pipeline"
              << std::setw(45) << "Conveyor belt pipeline"
              << "\n";

    std::cout << std::left << std::setw(18) << " "
              << std::right
              << std::setw(10) << "Total us"
              << std::setw(10) << "Nodes"
              << "   |   "
              << std::setw(10) << "Total us"
              << std::setw(10) << "Nodes"
              << "\n";

    std::cout << std::string(100, '-') << "\n";

    for (int depth = depth_step; depth <= max_depth; depth += depth_step) {

        std::string input = makeZeroInput(nq);

        double stdUsSum = 0.0, convUsSum = 0.0, nodesAfterSum = 0.0;
        int stdOkCount = 0, convOkCount = 0, reps_done = 0;

        for (int rep = 0; rep < REPS; ++rep) {
            std::string qasm = randomClifford(nq, depth, std::nullopt, std::nullopt, std::nullopt, 0.2);
            if (qasm.empty()) continue;

            GraphStats gsStd, gsConv;
            bool stdOk = true, convOk = true;
            double stdUs = 0.0, convUs = 0.0;

            // The paper reports that full initialization fails to complete
            // starting around circuit depth 100 (excessive memory/runtime);
            // catch that here instead of aborting the whole benchmark run.
            try {
                StageTiming tStd = benchmarkPipeline(qasm, input, gsStd, true, 1, false);
                stdUs = tStd.simulate_us;
            } catch (const std::exception& e) {
                stdOk = false;
                std::cerr << "  [standard failed] depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
            }

            try {
                StageTiming tConv = benchmarkPipeline(qasm, input, gsConv, true, 1, true);
                convUs = tConv.simulate_us;
            } catch (const std::exception& e) {
                convOk = false;
                std::cerr << "  [conveyor belt failed] depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
            }

            double nodesAfter = convOk ? gsConv.mbqc_nodes_after : gsStd.mbqc_nodes_after;

            csv << depth << "," << rep << "," << nodesAfter << ","
                << stdUs << "," << (stdOk ? 1 : 0) << ","
                << convUs << "," << (convOk ? 1 : 0) << "\n";

            if (stdOk)  { stdUsSum  += stdUs;  stdOkCount++;  }
            if (convOk) { convUsSum += convUs; convOkCount++; }
            nodesAfterSum += nodesAfter;
            ++reps_done;
        }

        if (reps_done == 0) continue;
        double nodesAfterAvg = nodesAfterSum / reps_done;
        double stdUsAvg  = (stdOkCount  > 0) ? stdUsSum  / stdOkCount  : 0.0;
        double convUsAvg = (convOkCount > 0) ? convUsSum / convOkCount : 0.0;

        std::cout << std::left  << std::setw(18) << depth
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(10) << stdUsAvg
                  << std::setw(10) << nodesAfterAvg
                  << "   |   "
                  << std::setw(10) << convUsAvg
                  << std::setw(10) << nodesAfterAvg
                  << "\n";
    }

    csv.close();
    std::cout << "\nRaw per-repetition results written to backend/test/results/full_vs_partial.csv\n";

    CHECK(true);
}


// =============================================
// BENCHMARK: Statevector vs TensorNetwork backend
// =============================================

TEST_CASE("Benchmark: Statevector vs TensorNetwork backend") {

    const int REPS = 8;

    auto makeZeroInput = [](int n) -> std::string {
        return "(1)|" + std::string(n, '0') + ">";
    };

    // Paired (qubits, depth) grid, growing together so the resulting
    // pattern's size, and with it the width of the entangled region the
    // simulator has to hold at once, grows smoothly from trivial to large.
    // Both backends run with the conveyor belt (dynamic allocation) enabled,
    // so this is a fair comparison of the two backends' representation of
    // the same activation schedule, not a repeat of the full-vs-partial
    // initialization comparison in Figure 8. Denser steps through the
    // 10-30 qubit range, where the crossover and the dense backend's
    // eventual failure both happen; coarser beyond that, where the dense
    // backend has already failed and the tensor-network backend is just
    // continuing its own, much gentler, exponential climb.
    std::vector<std::pair<int, int>> sizes = {
        {4, 10}, {5, 12}, {6, 15}, {7, 17}, {8, 20}, {9, 22}, {10, 25},
        {11, 27}, {12, 30}, {15, 40}, {18, 50}, {21, 60}, {24, 70},
    };

    std::ofstream csv = openResultsCsv(
        "sv_vs_tn.csv",
        "nq,depth,rep,nodes_after,edges_after,"
        "sv_us,sv_ok,sv_peak_qubits,sv_peak_amplitudes,"
        "tn_us,tn_ok,tn_peak_qubits,tn_peak_amplitudes");

    std::cout << "\n============================================================\n";
    std::cout << " Statevector vs TensorNetwork backend\n";
    std::cout << "============================================================\n\n";

    std::cout << std::left  << std::setw(10) << "Qubits"
              << std::setw(8)  << "Depth"
              << std::right
              << std::setw(10) << "Nodes"
              << std::setw(16) << "Statevector us"
              << std::setw(16) << "TensorNet us"
              << std::setw(12) << "Speedup"
              << "\n";
    std::cout << std::string(72, '-') << "\n";

    for (auto& [nq, depth] : sizes) {

        std::string input = makeZeroInput(nq);

        double simUsSum = 0.0, tnUsSum = 0.0, nodesAfterSum = 0.0;
        int simOkCount = 0, tnOkCount = 0, reps_done = 0;

        for (int rep = 0; rep < REPS; ++rep) {
            std::string qasm = randomClifford(nq, depth, std::nullopt, std::nullopt, std::nullopt, 0.2);
            if (qasm.empty()) continue;

            // Parsed, simplified and flow-found once and shared between both
            // backends, so they run on exactly the same pattern.
            QASMParser parser("", qasm);
            QuantumCircuit circ = parser.parse();
            ZXGraph zx = ZXGraph::fromQuantumCircuit(circ);
            MBQC_Graph graph = ZXtoMBQCGraph(zx);
            graph.simplify();

            double nodesAfter = graph.getSize();
            double edgesAfter = (int)graph.getAllEdges().size() / 2;

            PauliFlowResult flow = findPauliFlow(graph);
            if (!flow.ok) continue;

            bool svOk = true, tnOk = true;
            double svUs = 0.0, tnUs = 0.0;
            long long svPeakQubits = 0, tnPeakQubits = 0;
            long long svPeakAmps = 0, tnPeakAmps = 0;

            // Runs one backend to completion by stepping manually (instead
            // of simulateAll()) so we can sample its current qubit count and
            // memory footprint (Simulator::getCurrentQubitCount()/
            // getStoredAmplitudeCount() - cheap, no state copy) after every
            // step and keep the running peak, i.e. the actual memory the
            // backend held at its worst point during this run.
            auto runTracked = [&](const std::string& backend, long long& peakQ, long long& peakAmp) -> double {
                Simulator sim(graph, flow, true, input, 128, true, backend);
                auto t0 = Clock::now();
                while (!sim.isComplete()) {
                    sim.step(*sim.getReadyNodes().begin());
                    peakQ   = std::max(peakQ,   (long long)sim.getCurrentQubitCount());
                    peakAmp = std::max(peakAmp, sim.getStoredAmplitudeCount());
                }
                auto t1 = Clock::now();
                return (double)std::chrono::duration_cast<Micros>(t1 - t0).count();
            };

            // Dynamic allocation already keeps the backends' peak qubit
            // count well below the full pattern size (see Section IV-E2 of
            // the paper), but we still guard against the dense backend
            // running out of memory on an unexpectedly wide pattern rather
            // than aborting the whole benchmark run.
            try {
                svUs = runTracked("statevector", svPeakQubits, svPeakAmps);
            } catch (const std::exception& e) {
                svOk = false;
                std::cerr << "  [sv failed] nq=" << nq << " depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
            }

            try {
                tnUs = runTracked("tensornetwork", tnPeakQubits, tnPeakAmps);
            } catch (const std::exception& e) {
                tnOk = false;
                std::cerr << "  [tn failed] nq=" << nq << " depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
            }

            csv << nq << "," << depth << "," << rep << ","
                << nodesAfter << "," << edgesAfter << ","
                << svUs << "," << (svOk ? 1 : 0) << "," << svPeakQubits << "," << svPeakAmps << ","
                << tnUs << "," << (tnOk ? 1 : 0) << "," << tnPeakQubits << "," << tnPeakAmps << "\n";

            if (svOk) { simUsSum += svUs; simOkCount++; }
            if (tnOk) { tnUsSum  += tnUs; tnOkCount++;  }
            nodesAfterSum += nodesAfter;
            ++reps_done;
        }

        if (reps_done == 0) continue;
        double nodesAfter = nodesAfterSum / reps_done;
        double simUs = (simOkCount > 0) ? simUsSum / simOkCount : 0.0;
        double tnUs  = (tnOkCount  > 0) ? tnUsSum  / tnOkCount  : 0.0;

        double speedup = (tnUs > 0.0 && simOkCount > 0) ? (simUs / tnUs) : 0.0;

        std::cout << std::left  << std::setw(10) << nq
                  << std::setw(8)  << depth
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(10) << nodesAfter
                  << std::setw(16) << simUs
                  << std::setw(16) << tnUs
                  << std::setprecision(2)
                  << std::setw(11) << speedup << "x"
                  << "\n";
    }

    csv.close();
    std::cout << "\nRaw per-repetition results written to backend/test/results/sv_vs_tn.csv\n";

    CHECK(true);
}


// =============================================
// BENCHMARK: Node simplify() vs Edge greedyOptimizeEdges()
// =============================================

TEST_CASE("Benchmark: simplify vs greedyOptimizeEdges") {

    const int REPS = 8;

    // {28, 70} was tried and consistently threw std::bad_alloc even on the
    // tensor-network backend on a 14GB machine (under the 4GB ulimit used
    // for these runs), so the grid stops at {24, 60}, the largest size that
    // completes reliably. 12 points (not 10) so plot_node_vs_edge_reduction.py's
    // 3 size bins divide evenly: 4 grid points x REPS=8 = 32 repetitions
    // per bin, exactly, instead of an uneven 24/24/32 split.
    std::vector<std::pair<int, int>> sizes = {
        {4, 10}, {6, 15}, {8, 20}, {9, 22}, {10, 25}, {12, 30}, {14, 35},
        {16, 40}, {18, 45}, {20, 50}, {22, 55}, {24, 60},
    };

    // A planar (XY/XZ/YZ) node whose angle is not a multiple of pi/2 is non-Clifford.
    auto countNonClifford = [](const MBQC_Graph& g) -> int {
        int count = 0;
        for (int node = 0; node < g.getSize(); ++node) {
            auto [basis, angle] = g.getMeasurement(node);
            bool isPlanar = (basis == MeasurementBasis::XY ||
                             basis == MeasurementBasis::XZ ||
                             basis == MeasurementBasis::YZ);
            if (!isPlanar) continue;

            float a = normalize_radians((float)angle);
            bool isQuarterAngle = fAlmostEqual(fmod(a, (float)(M_PI / 2)), 0);
            if (!isQuarterAngle) ++count;
        }
        return count;
    };

    auto makeZeroInput = [](int n) -> std::string {
        return "(1)|" + std::string(n, '0') + ">";
    };

    std::ofstream csv = openResultsCsv(
        "node_vs_edge_reduction.csv",
        "nq,depth,rep,nodes_before,nonclifford_before,"
        "simp_nodes_after,simp_edges_after,simp_us,simp_sim_us,"
        "edge_nodes_after,edge_edges_after,edge_us,edge_sim_us");

    std::cout << "\n============================================================\n";
    std::cout << " simplify() vs greedyOptimizeEdges() -- simplify+simulate total\n";
    std::cout << "============================================================\n\n";

    std::cout << std::left  << std::setw(18) << " "
              << std::setw(40) << " Original "
              << std::setw(55) << "simplify() pipeline"
              << std::setw(55) << "greedyOptimizeEdges() pipeline"
              << "\n";

    std::cout << std::left  << std::setw(10) << "Qubits"
              << std::setw(8)  << "Depth"
              << std::right
              << std::setw(10) << "Nodes"
              << std::setw(12) << "NonCliff"
              << std::setw(9)  << "Nodes"
              << std::setw(9)  << "Edges"
              << std::setw(11) << "simp us"
              << std::setw(11) << "sim us"
              << std::setw(11) << "total us"
              << std::setw(9)  << "Nodes"
              << std::setw(9)  << "Edges"
              << std::setw(11) << "simp us"
              << std::setw(11) << "sim us"
              << std::setw(11) << "total us"
              << std::setw(11) << "Speedup"
              << "\n";
    std::cout << std::string(160, '-') << "\n";

    for (auto& [nq, depth] : sizes) {

        double simplifyUs = 0.0, edgesUs = 0.0;
        double simulateUsA = 0.0, simulateUsB = 0.0; // flow-finding + simulation, after each method
        double nodesBefore = 0.0, nonCliffordBefore = 0.0;
        double simplifyNodesAfter = 0.0, simplifyEdgesAfter = 0.0;
        double edgesNodesAfter = 0.0, edgesEdgesAfter = 0.0;
        int reps_done = 0;

        std::string inputState = makeZeroInput(nq);

        for (int rep = 0; rep < REPS; ++rep) {
            std::string qasm = randomClifford(nq, depth, 0.4, std::nullopt, std::nullopt, 0.4);
            if (qasm.empty()) continue;

            QASMParser parser("", qasm);
            QuantumCircuit circ = parser.parse();
            ZXGraph zx = ZXGraph::fromQuantumCircuit(circ);
            MBQC_Graph baseGraph = ZXtoMBQCGraph(zx);

            nodesBefore        += baseGraph.getSize();
            nonCliffordBefore  += countNonClifford(baseGraph);

            // --- Path A: simplify() then flow + simulate ---
            MBQC_Graph gSimplify = baseGraph.clone();
            auto t0 = Clock::now();
            gSimplify.simplify();
            auto t1 = Clock::now();

            simplifyNodesAfter += gSimplify.getSize();
            simplifyEdgesAfter += (int)gSimplify.getAllEdges().size() / 2;

            PauliFlowResult flowA = findPauliFlow(gSimplify);
            if (flowA.ok) {
                // Even the tensor-network backend can run out of memory on
                // an unusually dense random circuit at the larger grid
                // sizes; catch that rather than aborting the whole run.
                try {
                    Simulator simA(gSimplify, flowA, true, inputState, 128, true, "tensornetwork");
                    simA.simulateAll();
                } catch (const std::exception& e) {
                    std::cerr << "  [simplify-path sim failed] nq=" << nq << " depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
                }
            }
            auto t2 = Clock::now();

            // --- Path B: greedyOptimizeEdges() then flow + simulate ---
            MBQC_Graph gEdges = baseGraph.clone();
            auto t3 = Clock::now();
            gEdges.greedyOptimizeEdges();
            auto t4 = Clock::now();

            edgesNodesAfter += gEdges.getSize();
            edgesEdgesAfter += (int)gEdges.getAllEdges().size() / 2;

            PauliFlowResult flowB = findPauliFlow(gEdges);
            if (flowB.ok) {
                try {
                    Simulator simB(gEdges, flowB, true, inputState, 128, true, "tensornetwork");
                    simB.simulateAll();
                } catch (const std::exception& e) {
                    std::cerr << "  [edges-path sim failed] nq=" << nq << " depth=" << depth << " rep=" << rep << ": " << e.what() << "\n";
                }
            }
            auto t5 = Clock::now();

            double repSimplifyUs = std::chrono::duration_cast<Micros>(t1 - t0).count();
            double repSimUsA     = std::chrono::duration_cast<Micros>(t2 - t1).count(); // flow + simulate
            double repEdgesUs    = std::chrono::duration_cast<Micros>(t4 - t3).count();
            double repSimUsB     = std::chrono::duration_cast<Micros>(t5 - t4).count(); // flow + simulate

            simplifyUs  += repSimplifyUs;
            simulateUsA += repSimUsA;
            edgesUs     += repEdgesUs;
            simulateUsB += repSimUsB;

            csv << nq << "," << depth << "," << rep << ","
                << baseGraph.getSize() << "," << countNonClifford(baseGraph) << ","
                << gSimplify.getSize() << "," << (int)(gSimplify.getAllEdges().size() / 2) << ","
                << repSimplifyUs << "," << repSimUsA << ","
                << gEdges.getSize() << "," << (int)(gEdges.getAllEdges().size() / 2) << ","
                << repEdgesUs << "," << repSimUsB << "\n";

            ++reps_done;
        }

        if (reps_done == 0) continue;

        simplifyUs          /= reps_done;
        edgesUs              /= reps_done;
        simulateUsA          /= reps_done;
        simulateUsB          /= reps_done;
        nodesBefore          /= reps_done;
        nonCliffordBefore    /= reps_done;
        simplifyNodesAfter   /= reps_done;
        simplifyEdgesAfter   /= reps_done;
        edgesNodesAfter      /= reps_done;
        edgesEdgesAfter      /= reps_done;

        double totalA = simplifyUs + simulateUsA;
        double totalB = edgesUs + simulateUsB;
        double speedup = (totalB > 0.0) ? (totalB / totalA) : 0.0; // >1 means simplify() pipeline is faster overall

        std::cout << std::left  << std::setw(10) << nq
                  << std::setw(8)  << depth
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(10) << nodesBefore
                  << std::setw(12) << nonCliffordBefore
                  << std::setw(9)  << simplifyNodesAfter
                  << std::setw(9)  << simplifyEdgesAfter
                  << std::setw(11) << simplifyUs
                  << std::setw(11) << simulateUsA
                  << std::setw(11) << totalA
                  << std::setw(9)  << edgesNodesAfter
                  << std::setw(9)  << edgesEdgesAfter
                  << std::setw(11) << edgesUs
                  << std::setw(11) << simulateUsB
                  << std::setw(11) << totalB
                  << std::setprecision(2)
                  << std::setw(10) << speedup << "x"
                  << "\n";
    }

    csv.close();
    std::cout << "\nRaw per-repetition results written to backend/test/results/node_vs_edge_reduction.csv\n";

    CHECK(true);
}

