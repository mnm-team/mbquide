#include "doctest.h"
#include "utils.hpp"
#include "test_helpers.hpp"
#include "Simulator.hpp"
#include "Statevector.hpp"
#include "Tensornetwork.hpp"
#include "MBQC_Graph.hpp"
#include "Flow.hpp"
#include "ZX_Graph.hpp"
#include "ZX2MBQC.hpp"
#include "QASM_Parser.hpp"
#include "Quantum_Circuit.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>


// =====================================================================
// This file is *not* about proving one backend is faster. It exists to
// build intuition for how TensorNetworkSimulator actually behaves: it is
// not an SVD-truncated MPS, it is a disjoint union of dense blocks that
// get kron'd together whenever a CZ connects two blocks that were
// previously separate. So its speed relative to the plain statevector
// backend depends entirely on how *sparse* the entanglement structure of
// a scenario is - lots of small, independent blocks -> big win; a graph
// where everything eventually merges into one block -> no win (and some
// bookkeeping overhead on top). The block-structure printouts below make
// that mechanism visible instead of just reporting a timing number.
// =====================================================================

using Clock = std::chrono::high_resolution_clock;
using Micros = std::chrono::microseconds;

static double usSince(Clock::time_point t0, Clock::time_point t1) {
    return (double)std::chrono::duration_cast<Micros>(t1 - t0).count();
}

// ---------------------------------------------------------------------
// Terminal visualization of the tensor-network backend's block structure
// ---------------------------------------------------------------------
static void printBlockStructure(const TensorNetworkSimulator& tn, const std::string& label) {
    auto blocks = tn.getBlockStructure();
    int n = tn.get_num_qubits();

    std::cout << "      " << label << "\n      ";

    if (n == 0 || blocks.empty()) {
        std::cout << "(no active qubits)\n";
        return;
    }

    for (const auto& block : blocks) {
        std::cout << "[";
        for (size_t i = 0; i < block.size(); ++i) {
            std::cout << "q" << block[i];
            if (i + 1 < block.size()) std::cout << " ";
        }
        std::cout << "] ";
    }
    std::cout << "\n";

    size_t largest = std::max_element(blocks.begin(), blocks.end(),
        [](const auto& a, const auto& b) { return a.size() < b.size(); })->size();
    long long stored = tn.getStoredAmplitudeCount();
    long long dense = 1LL << n;

    std::cout << "      " << blocks.size() << " block(s), largest = " << largest
               << " qubit(s)  |  amplitudes stored: " << stored << " / " << dense
               << " dense  (" << std::fixed << std::setprecision(1)
               << (100.0 * (double)stored / (double)dense) << "%)\n";
}

static void printSectionHeader(const std::string& title) {
    std::cout << "\n============================================================\n"
               << " " << title << "\n"
               << "============================================================\n\n"
               << std::left << std::setw(34) << "Scenario"
               << std::right << std::setw(14) << "Statevector us"
               << std::setw(14) << "TensorNet us"
               << std::setw(10) << "Speedup" << "\n"
               << std::string(72, '-') << "\n";
}

static void printComparisonRow(const std::string& name, double svUs, double tnUs) {
    double speedup = (tnUs > 0.0) ? (svUs / tnUs) : 0.0;
    std::cout << std::left << std::setw(34) << name
               << std::right << std::fixed << std::setprecision(1)
               << std::setw(14) << svUs
               << std::setw(14) << tnUs
               << std::setprecision(2) << std::setw(9) << speedup << "x\n";
}


// =====================================================================
// PART 1: plain circuits, no MBQC involved - gates applied directly to
// both backends so the entanglement pattern is fully under our control.
// =====================================================================

template <typename Backend>
static void cnot(Backend& sim, int c, int t) {
    sim.H(t);
    sim.CZ(c, t);
    sim.H(t);
}

// Runs `build` (a generic lambda applying the same gate sequence to
// whichever backend it's given) on both backends, times it, checks the
// results agree, and prints the TN block structure at the end.
template <typename Build>
static void runSimpleScenario(const std::string& name, int n, Build build, int reps = 20) {
    double svUs = 0.0, tnUs = 0.0;
    TensorNetworkSimulator lastTn(0, false);

    for (int r = 0; r < reps; ++r) {
        StatevectorSimulator sv(n, false);
        TensorNetworkSimulator tn(n, false);

        auto t0 = Clock::now();
        build(sv);
        auto t1 = Clock::now();
        build(tn);
        auto t2 = Clock::now();

        svUs += usSince(t0, t1);
        tnUs += usSince(t1, t2);

        if (r == 0) {
            CHECK(TensorNetworkSimulator::isEqualUpToGlobalPhase(sv.get_statevector(), tn.get_statevector()));
            lastTn = tn;
        }
    }
    svUs /= reps;
    tnUs /= reps;

    printComparisonRow(name, svUs, tnUs);
    printBlockStructure(lastTn, "final block structure");
}

TEST_CASE("Scenario: simple circuits (no MBQC) - Statevector vs TensorNetwork backend") {
    const int n = 16;

    printSectionHeader("Simple circuits, " + std::to_string(n) + " qubits (no MBQC)");

    runSimpleScenario("Product state (H only)", n, [n](auto& sim) {
        for (int q = 0; q < n; ++q) sim.H(q);
    });

    runSimpleScenario("Disjoint Bell pairs", n, [n](auto& sim) {
        for (int q = 0; q + 1 < n; q += 2) {
            sim.H(q);
            sim.CZ(q, q + 1);
        }
    });

    runSimpleScenario("GHZ chain (linear CNOTs)", n, [n](auto& sim) {
        sim.H(0);
        for (int q = 0; q + 1 < n; ++q) cnot(sim, q, q + 1);
    });

    const int nDense = 12;
    runSimpleScenario("Fully-connected (all-pairs CZ)", nDense, [nDense](auto& sim) {
        for (int q = 0; q < nDense; ++q) sim.H(q);
        for (int i = 0; i < nDense; ++i)
            for (int j = i + 1; j < nDense; ++j)
                sim.CZ(i, j);
    }, /*reps=*/10);

    CHECK(true);
}

// Nearest-neighbour "brick wall" circuit: entanglement starts fully local
// and gradually spreads until every qubit lives in one block. Snapshots
// the TN block structure after every layer so the merge can be watched
// happening in the terminal.
TEST_CASE("Scenario: brick-wall light-cone growth (no MBQC)") {
    const int n = 16;
    const int layers = 6;

    std::cout << "\n============================================================\n"
              << " Brick-wall circuit, " << n << " qubits, " << layers << " layers\n"
              << " (watch the TN blocks merge as the light cone spreads)\n"
              << "============================================================\n";

    StatevectorSimulator sv(n, false);
    TensorNetworkSimulator tn(n, false);

    auto t0 = Clock::now();
    for (int layer = 0; layer < layers; ++layer) {
        for (int q = 0; q < n; ++q) sv.H(q);
        for (int q = layer % 2; q + 1 < n; q += 2) sv.CZ(q, q + 1);
    }
    auto t1 = Clock::now();

    for (int layer = 0; layer < layers; ++layer) {
        for (int q = 0; q < n; ++q) tn.H(q);
        for (int q = layer % 2; q + 1 < n; q += 2) tn.CZ(q, q + 1);
        printBlockStructure(tn, "after layer " + std::to_string(layer + 1) + "/" + std::to_string(layers));
    }
    auto t2 = Clock::now();

    CHECK(TensorNetworkSimulator::isEqualUpToGlobalPhase(sv.get_statevector(), tn.get_statevector()));
    std::cout << "\n";
    printComparisonRow("Brick-wall (" + std::to_string(layers) + " layers)", usSince(t0, t1), usSince(t1, t2));

    CHECK(true);
}


// =====================================================================
// PART 2: MBQC scenarios - graphs measured through the flow-driven
// Simulator, on both backends. Some graphs are built directly (full
// control over topology, no python dependency); the last scenario goes
// through the real QASM -> ZX -> MBQC pipeline like benchmark.cpp, at two
// different entangling-gate densities, to show how circuit connectivity
// (not qubit count) is what actually drives the TN backend's advantage.
// =====================================================================

static MBQC_Graph buildLinearClusterGraph(int n) {
    MBQC_Graph g(n, {0}, {n - 1});
    for (int i = 0; i < n - 1; ++i) g.addEdge(i, i + 1);
    for (int i = 0; i < n - 1; ++i) g.setMeasurement(i, MeasurementBasis::X, 0.0);
    return g;
}

// `numChains` independent linear cluster chains packed into one graph
// (no edges between chains) - the MBQC analogue of "disjoint Bell pairs".
static MBQC_Graph buildDisjointChainsGraph(int numChains, int chainLen) {
    int n = numChains * chainLen;
    std::vector<int> inputs, outputs;
    for (int c = 0; c < numChains; ++c) {
        inputs.push_back(c * chainLen);
        outputs.push_back(c * chainLen + chainLen - 1);
    }
    MBQC_Graph g(n, inputs, outputs);
    for (int c = 0; c < numChains; ++c) {
        int base = c * chainLen;
        for (int i = 0; i < chainLen - 1; ++i) g.addEdge(base + i, base + i + 1);
        for (int i = 0; i < chainLen - 1; ++i) g.setMeasurement(base + i, MeasurementBasis::X, 0.0);
    }
    return g;
}

// Rectangular cluster grid, all non-output nodes X-measured. Not every
// graph/basis assignment admits a Pauli flow, so the runner below checks
// flow.ok and skips gracefully instead of assuming it always works.
static MBQC_Graph buildGridClusterGraph(int rows, int cols) {
    int n = rows * cols;
    auto idx = [cols](int r, int c) { return r * cols + c; };
    MBQC_Graph g(n, {idx(0, 0)}, {idx(rows - 1, cols - 1)});
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) g.addEdge(idx(r, c), idx(r, c + 1));
            if (r + 1 < rows) g.addEdge(idx(r, c), idx(r + 1, c));
        }
    }
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            if (idx(r, c) != idx(rows - 1, cols - 1))
                g.setMeasurement(idx(r, c), MeasurementBasis::X, 0.0);
    return g;
}

// Star graph: one center connected to every leaf. Meant to be the "worst
// case" for the TN backend, since the center's very first activation
// pulls every leaf into one block.
static MBQC_Graph buildStarClusterGraph(int numLeaves) {
    int n = numLeaves + 1;
    int center = numLeaves;
    MBQC_Graph g(n, {0}, {center});
    for (int leaf = 0; leaf < numLeaves; ++leaf) g.addEdge(leaf, center);
    for (int leaf = 0; leaf < numLeaves; ++leaf) g.setMeasurement(leaf, MeasurementBasis::X, 0.0);
    return g;
}

// Runs the flow-driven Simulator on both backends, checks agreement,
// prints a timing row, and (optionally) snapshots the TN block structure
// as the simulation progresses.
static void runMBQCScenario(
    const std::string& name,
    const MBQC_Graph& graph,
    std::string inputState = "",
    int visualizeEveryKSteps = 0)
{
    PauliFlowResult flow = findPauliFlow(graph);
    if (!flow.ok) {
        std::cout << "  [skip] " << name << ": no valid Pauli flow for this topology/basis choice.\n";
        return;
    }

    auto t0 = Clock::now();
    Simulator sv(graph, flow, false, inputState, 128, true, "statevector");
    sv.simulateAll();
    auto t1 = Clock::now();

    Simulator tn(graph, flow, false, inputState, 128, true, "tensornetwork");
    int stepCount = 0;
    while (!tn.isComplete()) {
        auto ready = tn.getReadyNodes();
        if (ready.empty()) break;
        tn.step(*ready.begin());
        ++stepCount;
        if (visualizeEveryKSteps > 0 && stepCount % visualizeEveryKSteps == 0) {
            printBlockStructure(tn.getTensorNetworkSimulator(), "after step " + std::to_string(stepCount));
        }
    }
    auto t2 = Clock::now();

    if (visualizeEveryKSteps > 0) {
        printBlockStructure(tn.getTensorNetworkSimulator(), "final");
    }

    CHECK(TensorNetworkSimulator::isEqualUpToGlobalPhase(
        sv.getStatevectorSimulator().get_statevector(),
        tn.getTensorNetworkSimulator().get_statevector()));

    printComparisonRow(name, usSince(t0, t1), usSince(t1, t2));
}

TEST_CASE("Scenario: MBQC graph topologies - Statevector vs TensorNetwork backend") {
    printSectionHeader("MBQC graphs, hand-built topologies");

    runMBQCScenario("Linear cluster chain (n=20)", buildLinearClusterGraph(20), "", /*viz every*/ 5);
    runMBQCScenario("5x Disjoint chains (n=4 each)", buildDisjointChainsGraph(5, 4), "", 5);
    runMBQCScenario("Star graph (16 leaves)", buildStarClusterGraph(16), "", 0);
    runMBQCScenario("4x4 grid cluster", buildGridClusterGraph(4, 4), "", 4);

    CHECK(true);
}

TEST_CASE("Scenario: MBQC via QASM->ZX->MBQC pipeline, sparse vs dense entanglement") {
    printSectionHeader("Random Clifford circuits through the real pipeline");

    const int nq = 10;
    const int depth = 10;
    std::string input = "(1)|" + std::string(nq, '0') + ">";

    auto runPipelineScenario = [&](const std::string& name, double p_cnot) {
        std::string qasm = randomClifford(nq, depth, std::nullopt, std::nullopt, std::nullopt, p_cnot);
        if (qasm.empty()) {
            std::cout << "  [skip] " << name << ": could not generate circuit (python_venv missing?).\n";
            return;
        }
        QASMParser parser("", qasm);
        QuantumCircuit circ = parser.parse();
        ZXGraph zx = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph graph = ZXtoMBQCGraph(zx);
        graph.simplify();

        runMBQCScenario(name, graph, input, /*viz every*/ 0);
    };

    // Low p_cnot: circuit stays close to a product of small local patches
    // -> entanglement in the resulting MBQC graph should stay sparse.
    runPipelineScenario("p_cnot=0.1 (sparse entangling)", 0.1);

    // High p_cnot: circuit is dominated by two-qubit gates -> the MBQC
    // graph is densely connected, so the TN backend's blocks are expected
    // to merge into one almost immediately, same as the existing
    // benchmark.cpp result.
    runPipelineScenario("p_cnot=0.7 (dense entangling)", 0.7);

    CHECK(true);
}


// =====================================================================
// PART 3: entanglement-spectrum diagnostics - building intuition for
// whether SVD *truncation* (as opposed to the exact SVD splits the TN
// backend already performs internally at chi=0) would actually help for
// MBQC. Graph/cluster states are stabilizer states: on any bipartition
// their Schmidt spectrum is provably *flat* - every nonzero singular
// value has (close to) the same magnitude. That means there is no small
// tail to cut off: truncating a Pauli-measured MBQC computation either
// keeps the full rank (no compression at all) or discards weight that
// is *not* small (real, O(1) fidelity loss per cut). Non-Clifford
// measurement angles break the graph-state structure, so a spectrum
// that actually decays would be the sign that truncation has something
// to bite into. The prints below make that contrast directly visible
// instead of just asserting it.
// =====================================================================

// Rectangular cluster grid wired as a "computation": inputs are the
// first column, outputs the last column, one logical wire per row (the
// standard universal-MBQC cluster-state topology). Every non-output
// node is measured in `basis` at `angle`. Unlike buildGridClusterGraph
// above (single-corner output, fine for Pauli/X measurements only), a
// full output column reliably admits a flow for any measurement angle.
static MBQC_Graph buildComputeGridGraph(int rows, int cols, MeasurementBasis basis, double angle) {
    int n = rows * cols;
    auto idx = [cols](int r, int c) { return r * cols + c; };
    std::vector<int> inputs, outputs;
    for (int r = 0; r < rows; ++r) {
        inputs.push_back(idx(r, 0));
        outputs.push_back(idx(r, cols - 1));
    }
    MBQC_Graph g(n, inputs, outputs);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) g.addEdge(idx(r, c), idx(r, c + 1));
            if (r + 1 < rows) g.addEdge(idx(r, c), idx(r + 1, c));
        }
    }
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols - 1; ++c)
            g.setMeasurement(idx(r, c), basis, angle);
    return g;
}

// Same topology as buildComputeGridGraph, but each measured node cycles
// through a fixed pattern of *different* bases and angles instead of one
// uniform choice - closer to what an actual algorithm's measurement
// pattern looks like (a mix of Pauli corrections and several distinct
// rotation angles side by side) than the idealized single-angle sweep
// above.
static MBQC_Graph buildMixedBasisGridGraph(int rows, int cols) {
    static const std::vector<std::pair<MeasurementBasis, double>> pattern = {
        {MeasurementBasis::X, 0.0},
        {MeasurementBasis::XY, M_PI / 8.0},
        {MeasurementBasis::Y, 0.0},
        {MeasurementBasis::XY, 0.37 * M_PI},
        {MeasurementBasis::XY, -M_PI / 6.0},
        {MeasurementBasis::XY, M_PI / 3.0},
    };

    int n = rows * cols;
    auto idx = [cols](int r, int c) { return r * cols + c; };
    std::vector<int> inputs, outputs;
    for (int r = 0; r < rows; ++r) {
        inputs.push_back(idx(r, 0));
        outputs.push_back(idx(r, cols - 1));
    }
    MBQC_Graph g(n, inputs, outputs);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) g.addEdge(idx(r, c), idx(r, c + 1));
            if (r + 1 < rows) g.addEdge(idx(r, c), idx(r + 1, c));
        }
    }
    int k = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols - 1; ++c) {
            auto [basis, angle] = pattern[k % pattern.size()];
            g.setMeasurement(idx(r, c), basis, angle);
            ++k;
        }
    }
    return g;
}

// Prints the Schmidt singular-value spectrum across every internal cut
// of the largest currently-active block. Skips blocks above `maxQubits`
// since extracting the spectrum requires contracting the block to a
// dense vector (exponential; diagnostics only, never on the hot path).
static void printSingularValueSpectrum(const TensorNetworkSimulator& tn, const std::string& label, int maxQubits = 20) {
    auto blocks = tn.getBlockStructure();
    if (blocks.empty()) return;

    const auto& largest = *std::max_element(blocks.begin(), blocks.end(),
        [](const auto& a, const auto& b) { return a.size() < b.size(); });

    std::cout << "      " << label << " - largest block: " << largest.size() << " qubit(s)";
    if ((int)largest.size() > maxQubits) {
        std::cout << "  (skipping spectrum - too big to contract densely)\n";
        return;
    }
    std::cout << "\n";

    int blockId = tn.getBlockId(largest.front());
    auto spectra = tn.getSingularValueSpectrum(blockId);

    for (size_t cut = 0; cut < spectra.size(); ++cut) {
        const auto& S = spectra[cut];
        double maxS = S.empty() ? 0.0 : S.front();
        double eps = std::max(1e-12, maxS * 1e-10);

        int rank = 0;
        double entropy = 0.0;
        double minKept = maxS;
        for (double s : S) {
            if (s <= eps) continue;
            ++rank;
            minKept = std::min(minKept, s);
            double p = s * s;
            if (p > 1e-300) entropy -= p * std::log2(p);
        }
        double flatness = (rank > 0 && maxS > 0.0) ? (minKept / maxS) : 0.0;

        std::cout << "        cut " << cut << "|" << (cut + 1)
                   << "  rank=" << std::setw(4) << rank
                   << "  entropy=" << std::fixed << std::setprecision(3) << std::setw(6) << entropy << " bits"
                   << "  min/max sigma=" << std::setprecision(4) << flatness;
        if (rank > 1 && flatness > 0.99) std::cout << "  <- FLAT (stabilizer-like: truncating below full rank loses O(1) fidelity)";
        std::cout << "\n";
    }
}

// Steps `sim` through the flow-driven MBQC computation, printing the
// entanglement spectrum of the largest block every `everyKSteps` steps.
static void runSpectrumScenario(const std::string& name, const MBQC_Graph& graph, int everyKSteps = 5) {
    PauliFlowResult flow = findPauliFlow(graph);
    if (!flow.ok) {
        std::cout << "  [skip] " << name << ": no valid Pauli flow for this topology/basis choice.\n";
        return;
    }

    std::cout << "\n  --- " << name << " ---\n";
    Simulator tn(graph, flow, false, "", 128, true, "tensornetwork");
    int step = 0;
    while (!tn.isComplete()) {
        auto ready = tn.getReadyNodes();
        if (ready.empty()) break;
        tn.step(*ready.begin());
        ++step;
        if (everyKSteps > 0 && step % everyKSteps == 0)
            printSingularValueSpectrum(tn.getTensorNetworkSimulator(), "after step " + std::to_string(step));
    }
    printSingularValueSpectrum(tn.getTensorNetworkSimulator(), "final");
}

TEST_CASE("Scenario: entanglement spectrum across MBQC cuts - intuition for MPS truncation") {
    printSectionHeader("Singular-value spectra: is MBQC entanglement compressible?");

    // Sparse: a long linear cluster chain. Every cut only ever separates
    // two directly-neighbouring qubits, so the Schmidt rank across any
    // cut of a 1D cluster state is at most 2 - the block stays tiny no
    // matter how long the chain gets.
    runSpectrumScenario("Sparse: 30-node linear cluster chain (X-measured)", buildLinearClusterGraph(30), 10);

    // Dense: a 2D grid cluster wired as a "computation" - inputs are the
    // first column, outputs the last column, one logical wire per row
    // (the standard universal-MBQC cluster topology). A single-corner
    // output (as buildGridClusterGraph above uses) only admits a flow
    // for Pauli measurements; a full output column admits one for any
    // angle, which is what the non-Clifford variant below needs.
    runSpectrumScenario("Dense: 4x5 grid cluster, X-measured (Clifford)",
        buildComputeGridGraph(4, 5, MeasurementBasis::X, 0.0), 5);

    // Same topology, but every measured node uses a non-Clifford XY
    // angle (pi/4, a T-gate-like angle) instead of X. This breaks the
    // graph-state structure, so if truncation is ever going to have
    // something to bite into for MBQC, it should show up here as actual
    // decay instead of a flat spectrum.
    runSpectrumScenario("Dense: 4x5 grid cluster, XY(pi/4)-measured (non-Clifford)",
        buildComputeGridGraph(4, 5, MeasurementBasis::XY, M_PI / 4.0), 5);

    // Larger non-Clifford instance (48 nodes vs. 20 above) for more
    // confidence that the decay above wasn't a small-graph fluke. Blocks
    // stay well under the dense-contraction cap here too, so the whole
    // spectrum keeps printing instead of getting skipped.
    runSpectrumScenario("Larger: 6x8 grid cluster, XY(pi/8)-measured (non-Clifford)",
        buildComputeGridGraph(6, 8, MeasurementBasis::XY, M_PI / 8.0), 8);

    // Non-uniform: same-shaped grid, but each node cycles through a
    // fixed pattern of *different* bases/angles (X, Y, and several
    // distinct XY rotations) instead of one repeated angle - checking
    // that the decay seen above isn't an artifact of every node sharing
    // the exact same angle.
    runSpectrumScenario("Mixed bases/angles: 4x6 grid cluster (non-uniform)",
        buildMixedBasisGridGraph(6, 8), 6);

    CHECK(true);
}


// =====================================================================
// PART 4: entanglement spectrum of a *real* circuit, not a hand-built
// cluster topology - a randomly generated Clifford+T circuit pushed
// through the actual QASM -> ZX -> MBQC pipeline (same machinery as the
// pipeline scenario in PART 2), with p_t > 0 so it actually contains
// non-Clifford T gates. This is the realistic case the flat-vs-decaying
// spectra above were building intuition for: does a circuit an MBQC
// backend would plausibly be asked to run actually have compressible
// entanglement, or is it flat like the pure-Pauli graphs?
// =====================================================================

TEST_CASE("Scenario: entanglement spectrum of a random Clifford+T circuit via the pipeline") {
    printSectionHeader("Singular-value spectra: a random Clifford+T circuit through the real pipeline");

    const int nq = 8;
    const int depth = 60;

    // p_t > 0 guarantees non-Clifford T gates are actually present;
    // p_cnot pushed up so the circuit actually entangles enough to have
    // an interesting spectrum (low p_cnot mostly produced rank-1 cuts -
    // an almost-product circuit). p_s/p_hsh left unset so pyzx fills in
    // the rest of the gate mix itself.
    std::string qasm = randomClifford(nq, depth, 0.2, std::nullopt, std::nullopt, 0.5);
    if (qasm.empty()) {
        std::cout << "  [skip] random Clifford+T circuit: could not generate circuit (python_venv missing?).\n";
        CHECK(true);
        return;
    }

    QASMParser parser("", qasm);
    QuantumCircuit circ = parser.parse();
    ZXGraph zx = ZXGraph::fromQuantumCircuit(circ);
    MBQC_Graph graph = ZXtoMBQCGraph(zx);
    graph.simplify();

    // Every step, not every K like the hand-built topologies above: the
    // decay this circuit produces is transient - the affected block
    // often gets measured through within just a few steps, so sampling
    // every 5 steps (like the sparser scenarios above) mostly missed it
    // during tuning.
    runSpectrumScenario(
        "Random Clifford+T circuit (" + std::to_string(nq) + "q, depth " + std::to_string(depth) + ", p_t=0.2, p_cnot=0.5)",
        graph, 1);

    CHECK(true);
}
