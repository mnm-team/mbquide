#ifndef MBQC_SIMULATOR_HPP
#define MBQC_SIMULATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <variant>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "MBQC_Graph.hpp"
#include "Flow.hpp"
#include "utils.hpp"
#include "Statevector.hpp"
#include "Tensornetwork.hpp"


/// Which concrete simulation backend a Simulator/SimulatorBackendHandle uses: dense StatevectorSimulator or TensorNetworkSimulator.
enum class SimulatorBackendType {
    Statevector,
    TensorNetwork
};

/// Parses a SimulatorBackendType from a name. Accepts `"statevector"`/`"sv"` and `"tensornetwork"`/`"tn"` (case-insensitive).
/// @throws std::invalid_argument if `backend` doesn't match a known name.
inline SimulatorBackendType parseSimulatorBackendType(const std::string& backend) {
    std::string b = backend;
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    if (b == "statevector" || b == "sv") return SimulatorBackendType::Statevector;
    if (b == "tensornetwork" || b == "tn") return SimulatorBackendType::TensorNetwork;
    throw std::invalid_argument("Simulator: unknown backend '" + backend + "' (expected statevector/sv or tensornetwork/tn)");
}

/**
 * @brief Wraps whichever concrete simulator backend
 * (StatevectorSimulator or TensorNetworkSimulator) is active behind a single
 * set of calls, dispatched via `std::variant`/`std::visit`, so Simulator
 * doesn't need to know which one it's talking to.
 */
class SimulatorBackendHandle {
public:
    SimulatorBackendHandle() = default;

    /// (Re)initializes the active backend as a fresh `n`-qubit `|00...0>` state of the given type.
    void init(SimulatorBackendType type, int n, bool random) {
        backendType = type;
        switch (type) {
            case SimulatorBackendType::Statevector:
                impl = StatevectorSimulator(n, random);
                break;
            case SimulatorBackendType::TensorNetwork:
                impl = TensorNetworkSimulator(n, random);
                break;
        }
    }

    /// Which backend type is currently active.
    SimulatorBackendType getBackendType() const { return backendType; }

    /// The active backend's current qubit count.
    int get_num_qubits() const {
        return std::visit([](auto& s) { return s.get_num_qubits(); }, impl);
    }

    /// Appends a new `|+>` qubit on the active backend; see StatevectorSimulator::add_qubit_plus()/TensorNetworkSimulator::add_qubit_plus().
    int add_qubit_plus() {
        return std::visit([](auto& s) { return s.add_qubit_plus(); }, impl);
    }

    /// Applies a controlled-Z gate on the active backend.
    void CZ(int u, int v) {
        std::visit([&](auto& s) { s.CZ(u, v); }, impl);
    }

    /// Applies a Hadamard gate on the active backend.
    void H(int q) { std::visit([&](auto& s) { s.H(q); }, impl); }
    /// Applies a Pauli-Z gate on the active backend.
    void Z(int q) { std::visit([&](auto& s) { s.Z(q); }, impl); }
    /// Applies an S gate on the active backend.
    void S(int q) { std::visit([&](auto& s) { s.S(q); }, impl); }

    /// Reorders qubits on the active backend; see StatevectorSimulator::reorderQubits()/TensorNetworkSimulator::reorderQubits().
    void reorderQubits(const std::vector<int>& permutation) {
        std::visit([&](auto& s) { s.reorderQubits(permutation); }, impl);
    }

    /// Measures a qubit in an MBQC basis/angle on the active backend; see StatevectorSimulator::measure_qubit_in_basis()/TensorNetworkSimulator::measure_qubit_in_basis().
    int measure_qubit_in_basis(int q, MeasurementBasis basis, double angle) {
        return std::visit([&](auto& s) { return s.measure_qubit_in_basis(q, basis, angle); }, impl);
    }

    /// Overwrites the active backend's entire state; see StatevectorSimulator::setState()/TensorNetworkSimulator::setState().
    void setState(const StatevectorSimulator::VectorC& state) {
        std::visit([&](auto& s) { s.setState(state); }, impl);
    }

    /// The active backend's current state as a bra-ket string.
    std::string getStatevectorBraKet() const {
        return std::visit([](auto& s) { return s.getStatevectorBraKet(); }, impl);
    }

    /// The active backend's current state as JSON.
    json toJson() const {
        return std::visit([](auto& s) { return s.toJson(); }, impl);
    }

    /// The active backend as a StatevectorSimulator. Only valid when `getBackendType() == SimulatorBackendType::Statevector`.
    StatevectorSimulator getStatevectorSimulator() const {
        return std::get<StatevectorSimulator>(impl);
    }

    /// The active backend as a TensorNetworkSimulator. Only valid when `getBackendType() == SimulatorBackendType::TensorNetwork`.
    TensorNetworkSimulator getTensorNetworkSimulator() const {
        return std::get<TensorNetworkSimulator>(impl);
    }

private:
    SimulatorBackendType backendType = SimulatorBackendType::Statevector;
    std::variant<StatevectorSimulator, TensorNetworkSimulator> impl;
};


/**
 * @brief Drives step-by-step execution of an MBQC_Graph pattern given a
 * PauliFlowResult, on top of a SimulatorBackendHandle.
 *
 * Owns a clone of the graph (so measurement corrections can be rewritten
 * in place — see rotateGraphNodeX()/rotateGraphNodeZ() — without touching
 * the session's own graph) and tracks, per vertex: whether it has been
 * *activated* (allocated a backend qubit — see activateNode()/
 * activateAllNecessary()), whether it is *ready to measure* (its X/Z
 * correction dependencies are resolved — recomputeReadyToMeasure()), and
 * whether it has been *measured*. step() measures one ready vertex,
 * applies the flow's Pauli corrections to every dependent vertex per the
 * "strong uniform stepwise determinism" result of <https://arxiv.org/abs/2410.23439>, then
 * (if `conveyorBelt` is set) activates whatever newly became necessary —
 * so at any point only the qubits actually needed so far are held in the
 * backend, rather than the whole pattern at once. simulateAll()/
 * runAndGetOutput() drive this to completion in one call.
 *
 * Constructed once per REST API session's simulation (see `/api/sim` in
 * `server.cpp`), from the session's current MBQC_Graph and PauliFlowResult.
 */
class Simulator {

private:
    MBQC_Graph graph;
    PauliFlowResult flow;
    bool randomMeasurements;
    int totalNodes;
    int numInputNodes;
    int maxVecSizeJSON;
    bool conveyorBelt;
    std::string inputStateString;
    SimulatorBackendType backendType;
    SimulatorBackendHandle backendSim;

    std::unordered_map<int, int> measurementOutcomes;
    std::unordered_map<int, std::set<std::string>> appliedCorrections;
    std::unordered_set<int> measured;
    std::unordered_set<int> activeNodes;
    std::unordered_set<int> deactivatedNodes;
    std::set<std::pair<int, int>> activeEdges;
    std::unordered_map<int, std::unordered_set<int>> Xdependencies;
    std::unordered_map<int, std::unordered_set<int>> Zdependencies;
    std::set<int> readyToMeasure;

    std::vector<int> qubitToGraphNode;
    // Reverse lookup of qubitToGraphNode: the backend qubit index currently
    // holding graph node n, or -1 if n has no qubit right now (not yet
    // activated, or already measured/traced out).
    int graphNodeToQubit(int n) {
        auto it = std::find(qubitToGraphNode.begin(), qubitToGraphNode.end(), n);
        if (it != qubitToGraphNode.end()) {
            int q = it - qubitToGraphNode.begin();
            return q;
        }
        return -1;  // means the qubit for this node does not exist
    }

    // Rewrites graph node u's measurement (or, for an output, its
    // OutputAdjustmentMap) to fold in a Z correction, i.e. what measuring u
    // would see after an X gate on its qubit - without touching the qubit
    // itself. Used to defer corrections symbolically (see class docs).
    void rotateGraphNodeZ(int u) {
        auto [basis, angle] = graph.getMeasurement(u);
        switch (basis) {
            case MeasurementBasis::X:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::Y:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::Z:
                break;
            case MeasurementBasis::XY:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::YZ:
                angle = -angle;
                break;
            case MeasurementBasis::XZ:
                angle = -angle;
                break;
            case MeasurementBasis::OUTPUT:
                graph.getOutputAdjustment(u).adjustOutput("Z");
                break;
            default:
                break;
        }
        graph.setMeasurement(u, basis, angle);
    }

    // Same as rotateGraphNodeZ(), but for folding in an X correction.
    void rotateGraphNodeX(int u) {
        auto [basis, angle] = graph.getMeasurement(u);
        switch (basis) {
            case MeasurementBasis::X:
                break;
            case MeasurementBasis::Y:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::Z:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::XY:
                angle = -angle;
                break;
            case MeasurementBasis::YZ:
                angle = angle + M_PI;
                break;
            case MeasurementBasis::XZ:
                angle = -angle;
                break;
            case MeasurementBasis::OUTPUT:
                graph.getOutputAdjustment(u).adjustOutput("X");
                break;
            default:
                break;
        }
        graph.setMeasurement(u, basis, angle);
    }

    // Dispatches to rotateGraphNodeX()/rotateGraphNodeZ() by axis ("X" or "Z").
    void rotateGraphNode(int u, std::string axis) {
        if (axis == "Z") {
            rotateGraphNodeZ(u);
        } else if (axis == "X") {
            rotateGraphNodeX(u);
        } else {
            throw std::runtime_error("Not implemented other graph node rotations than X and Z");
        }
    }


    // Returns true if a correction of 'corrType' has no effect on the
    // measurement outcome of 'node' given its basis.
    //
    // Implementing the impact of https://arxiv.org/pdf/2207.09368v4 2.2
    // -> A measurement impacts a vertex if the action of the correction anticommutes with one Pauli element of λ (MeasurementBasis)
    //
    // corrType is one of {"X", "Z", "Y"}
    bool correctionHasNoImpact(int node, char corrType) const {
        auto [basis, angle] = graph.getMeasurement(node);

        angle = normalize_radians(angle);

        if (corrType != 'X' && corrType != 'Z' && corrType !='Y')  {
            std::cerr << "Simulator: correctionType can only be X, Z or Y and not " << corrType << "!\n";
        }

        switch (basis) {
            case MeasurementBasis::OUTPUT:
                return false;
            case MeasurementBasis::X:
                return corrType == 'X';
            case MeasurementBasis::Z:
                return corrType == 'Z';
            case MeasurementBasis::Y:
                return corrType == 'Y';
            case MeasurementBasis::XY:
                if (corrType == 'Z') return false;
                if (fAlmostEqual(angle, 0) || fAlmostEqual(angle, M_PI)) {
                    return corrType == 'X';
                }
                if (fAlmostEqual(angle, M_PI/2) || fAlmostEqual(angle, 3 * M_PI/2)) {
                    return corrType == 'Y';
                }
                return false;
            case MeasurementBasis::XZ:
                if (corrType == 'Y') return false;
                if (fAlmostEqual(angle, 0) || fAlmostEqual(angle, M_PI)) {
                    return corrType == 'Z';
                }
                if (fAlmostEqual(angle, M_PI/2) || fAlmostEqual(angle, 3 * M_PI/2)) {
                    return corrType == 'X';
                }
                return false;
            case MeasurementBasis::YZ:
                if (corrType == 'X') return false;
                if (fAlmostEqual(angle, 0) || fAlmostEqual(angle, M_PI)) {
                    return corrType == 'Z';
                }
                if (fAlmostEqual(angle, M_PI/2) || fAlmostEqual(angle, 3 * M_PI/2)) {
                    return corrType == 'Y';
                }
                return false;
            default:
                return false;
        }
    }

    // Whether u has any neighbor that is currently active or was already
    // deactivated (i.e. measured) - used to decide whether u needs to be
    // activated to let its neighbors' edges be formed.
    bool neighboringActivated(int u) {
        std::vector<int> neighbors = graph.getNeighbors(u);
        for (int n : graph.getNeighbors(u)) {
            if (isActive(n)) return true;
            if (wasDeactivated(n)) return true;
        }
        return false;
    }

    // A dependency set is "resolved" for readiness purposes once the only
    // entry left (if any) is the node's own self-correction: that entry can
    // never actually be applied, since by the time it would fire the node
    // has already been measured.
    static bool depsResolved(int node, const std::unordered_set<int>& deps) {
        return deps.empty() || (deps.size() == 1 && deps.count(node));
    }

    // Recompute which nodes are ready to be measured.
    // A non-input, non-done node is ready when, for each correction axis
    // (X and Z) independently, either:
    //   (a) that axis's pending dependencies are resolved (measured or self-only), OR
    //   (b) that axis's correction type has no impact on the node's basis,
    // As a special case, if the X- and Z-dependencies are identical, the
    // combined correction acts as Y (up to phase), so it's also enough for
    // that to be irrelevant to the basis even if neither axis alone is resolved.
    void recomputeReadyToMeasure(int u) {

        for (int node = 0; node < totalNodes; ++node) {

            if (isDone(node)) continue;

            const auto& xDeps = Xdependencies[node];
            const auto& zDeps = Zdependencies[node];

            const bool xOk = depsResolved(node, xDeps) || correctionHasNoImpact(node, 'X');
            const bool zOk = depsResolved(node, zDeps) || correctionHasNoImpact(node, 'Z');

            const bool yCorrIrrel =
                xDeps == zDeps && correctionHasNoImpact(node, 'Y');  // XZ ~ Y: irrelevant together even if pending

            if ((xOk && zOk) || yCorrIrrel) {
                readyToMeasure.insert(node);
            }
        }
    }




public:
    Simulator() = default;
    /**
     * @brief Constructs a simulator for `g` under Pauli flow `flow`.
     * @param g The MBQC graph to execute (cloned internally).
     * @param flow A Pauli flow for `g` (see findPauliFlow()); must have `flow.ok == true`.
     * @param random Whether measurements sample outcomes randomly (Born rule) or deterministically return 0.
     * @param inputState Optional bra-ket string (see StatevectorSimulator::parseBraKet()) to initialize the input qubits to instead of `|00...0>`.
     * @param maxVecSizeJSON Above this many amplitudes, toJson()/reorderQubitsCanonically() skip materializing/reordering the full statevector (it's still simulated correctly, just not serialized/canonicalized on every step).
     * @param conveyorBelt If true, activateAllNecessary() runs automatically after construction and after every step() (streaming qubit allocation); if false, activateAll() runs once upfront (every qubit allocated immediately).
     * @param backend Which SimulatorBackendType to use (see parseSimulatorBackendType()).
     */
    Simulator(const MBQC_Graph& g, const PauliFlowResult& flow, bool random = true, std::string inputState = "", int maxVecSizeJSON = 128, bool conveyorBelt = true, std::string backend = "tensornetwork")
        : graph(g.clone()), flow(flow), randomMeasurements(random), inputStateString(inputState), maxVecSizeJSON(maxVecSizeJSON), conveyorBelt(conveyorBelt), backendType(parseSimulatorBackendType(backend))
    {
        if (!flow.ok) {
            std::cerr << "Cannot create simulator from bad pauli flow!\n";
        }

        // Calculate sizse
        totalNodes = graph.getSize();
        numInputNodes = graph.getInputs().size();

        // Reserve stuff
        measured.reserve(totalNodes);
        measurementOutcomes.reserve(totalNodes);
        activeNodes.reserve(totalNodes);

        // Init everything
        initStatevector(inputStateString);

        // Build reverse flow dependencies
        for (const auto& [node, deps] : flow.corrf) {
            for (int dep : deps) {
                Xdependencies[dep].insert(node);
            }
        }
        for (const auto& [node, deps] : flow.oddNcorrf) {
            for (int dep : deps) {
                Zdependencies[dep].insert(node);
            }
        }

        // Start by making the inputs readyToMeasure
        for (int i : graph.getInputs()) {
            readyToMeasure.insert(i);
        }

        conveyorBelt ? activateAllNecessary() : activateAll();

    }

    /// The current backend state as a bra-ket string.
    std::string getStatevectorBraKet() const {
        return backendSim.getStatevectorBraKet();
    }

    /// The active backend as a StatevectorSimulator. Only valid when `getBackendType() == SimulatorBackendType::Statevector`.
    StatevectorSimulator getStatevectorSimulator() const {
        return backendSim.getStatevectorSimulator();
    }

    /// The active backend as a TensorNetworkSimulator. Only valid when `getBackendType() == SimulatorBackendType::TensorNetwork`.
    TensorNetworkSimulator getTensorNetworkSimulator() const {
        return backendSim.getTensorNetworkSimulator();
    }

    /// Which SimulatorBackendType this simulator is using.
    SimulatorBackendType getBackendType() const {
        return backendType;
    }

    /// Serializes the simulator's full state for the REST API: the graph, flow, ready/measured node sets, measurement outcomes, active edges/nodes, and (if small enough — see `maxVecSizeJSON`) the current statevector.
    json toJson() const {
        json j;

        j["graph"] = graph.toJson();
        j["flow"] = PauliFlowResultToJson(flow);


        j["readyToMeasure"] = readyToMeasure;
        j["measured"] = measured;
        j["outcomes"] = measurementOutcomes;
        if (1 << backendSim.get_num_qubits() > maxVecSizeJSON) {
            j["statevector"] = {};
        } else {
            j["statevector"] = backendSim.toJson();
        }
        j["activeEdges"] = activeEdges;

        // The reverse of qubitToGraph has already the right order of statevecotr
        std::vector<int> active = qubitToGraphNode;
        std::reverse(active.begin(), active.end());
        j["activeNodes"] = active;

        return j;
    }

    /// Updates `qubitToGraphNode` bookkeeping after backend qubit `q` was traced out (removed), shifting every higher qubit index down by one to match.
    void tracedOutQubit(int q) {
        if (q < 0 || q >= qubitToGraphNode.size()) {
            std::cerr << "Invalid qubit index!" << std::endl;
            return;
        }

        // shift all qubits that are greater than the deleted qubit
        for (int i = q + 1; i < qubitToGraphNode.size(); ++i) {
            qubitToGraphNode[i-1] = qubitToGraphNode[i];
        }

        // remove last qubit (redundant after shift)
        qubitToGraphNode.pop_back();
    }

    /// Whether `nodeId` is currently ready to be measured (step()-able).
    bool isReady(int nodeId) const {
        return readyToMeasure.count(nodeId) > 0;
    }

    /// The set of vertex ids currently ready to be measured.
    std::set<int> getReadyNodes() const {
        return readyToMeasure;
    }

    /// Whether `nodeId` is finished: measured (for a non-output), or has no pending output adjustment left to apply (for an output — see OutputAdjustmentMap::isStandard()).
    bool isDone(int nodeId) const {

        if (graph.isOutput(nodeId)) {
            const auto oa = graph.getOutputAdjustment(nodeId);
            if (!oa.isStandard()) {
                return false;
            } else {
                return true;
            }
        }

        return isMeasured(nodeId);
    }

    /// Whether `nodeId` has already been measured (step()-ed).
    bool isMeasured(int nodeId) const {
        return measured.count(nodeId) > 0;
    }

    /// Whether `nodeId` currently holds a backend qubit (was activated and hasn't been measured/traced out yet).
    bool isActive(int nodeId) const {
        return activeNodes.count(nodeId) > 0;
    }

    /// Whether `nodeId` was activated and has since been measured/deactivated.
    bool wasDeactivated(int nodeId) const {
        return deactivatedNodes.count(nodeId) > 0;
    }

    /// Whether the CZ for graph edge `(u, v)` has already been applied on the backend.
    bool isEdgeActive(int u, int v) const {
        return activeEdges.find({u, v}) != activeEdges.end() || activeEdges.find({v, u}) != activeEdges.end();
    }

    /// Allocates a fresh `|+>` backend qubit for `nodeId`, if it doesn't have one already (no-op if already active or already deactivated).
    void activateNode(int nodeId) {
        if (isActive(nodeId)) return;
        if (wasDeactivated(nodeId)) return;
        int q = backendSim.add_qubit_plus();
        if (q != qubitToGraphNode.size()) std::cerr << "Activated new node but ID is not correct!\n";
        qubitToGraphNode.insert(qubitToGraphNode.begin(), nodeId);
        activeNodes.insert(nodeId);
    }

    /// Applies the CZ for graph edge `(u, v)` on the backend, if not already applied. Both `u` and `v` must already be active.
    void activateEdge(int u, int v) {
        if (isEdgeActive(u, v)) return;
        if (!isActive(u)) {
            std::cerr << "Simulator: Edge cannot be activated as node " << u << " is not Activated.\n";
            return;
        }
        if (!isActive(v)) {
            std::cerr << "Simulator: Edge cannot be activated as node " << v << " is not Activated.\n";
            return;
        }
        backendSim.CZ(graphNodeToQubit(u), graphNodeToQubit(v));
        activeEdges.insert({u,v});
    }

    /// Activates every vertex and edge in the graph up front (used when `conveyorBelt` is false), then canonicalizes qubit order (reorderQubitsCanonically()).
    void activateAll() {
        for (int r = 0; r < totalNodes; r++) {
            activateNode(r);
            for (int n : graph.getNeighbors(r)) {
                activateNode(n);
                activateEdge(r, n);
            }
        }
        reorderQubitsCanonically();
    }

    /// Activates every vertex currently in `readyToMeasure` and their neighbors/incident edges (streaming allocation, used when `conveyorBelt` is true — called after construction and after every step()), then canonicalizes qubit order.
    void activateAllNecessary() {
        for (int r : readyToMeasure) {
            activateNode(r);
            for (int n : graph.getNeighbors(r)) {
                activateNode(n);
                activateEdge(r, n);
            }
        }
        reorderQubitsCanonically();
    }

    /// (Re)initializes the backend with `numInputNodes` qubits, optionally setting a custom input state, and marks the graph's input vertices active.
    void initStatevector(std::string inputStateString = "") {
        backendSim.init(backendType, numInputNodes, randomMeasurements);

        if (!inputStateString.empty()) {
            auto inputState = StatevectorSimulator::parseBraKet(inputStateString);
            backendSim.setState(inputState);
        }

        std::vector<int> inputs = graph.getInputs();
        std::sort(inputs.begin(), inputs.end(), std::greater<int>());

        qubitToGraphNode.reserve(numInputNodes);
        for (int i : inputs) {
            qubitToGraphNode.insert(qubitToGraphNode.begin(), i);
            activeNodes.insert(i);
        }
    }

    /**
     * @brief Executes one measurement step: measures `nodeId` (or, if it's an
     * output, writes out its accumulated OutputAdjustmentMap instead), then
     * propagates Pauli corrections and recomputes readiness.
     *
     * On an unwanted ("1") outcome, applies an X correction to every vertex
     * in `flow.corrf[nodeId]` and a Z correction to every vertex in the odd
     * neighborhood of that set (rotateGraphNode()), following the strong
     * uniform stepwise determinism result of <https://arxiv.org/abs/2410.23439> (p. 5). Then
     * clears `nodeId` from every other vertex's pending dependencies,
     * recomputes `readyToMeasure` (recomputeReadyToMeasure()), and — if
     * `conveyorBelt` is set — activates whatever newly became necessary
     * (activateAllNecessary()).
     *
     * @param nodeId The vertex to measure/finalize. Must be both ready
     * (isReady()) and active (isActive()).
     * @return `true` on success; `false` (with a diagnostic to stderr) if
     * `nodeId` wasn't ready/active/present as expected.
     */
    bool step(int nodeId) {

        if (!isReady(nodeId)) {
            std::cerr << "Node " << nodeId << " is not ready to be measured.\n";
            return false;
        }
        if (!isActive(nodeId)) {
            std::cerr << "Node " << nodeId << " was not activated yet.\n";
            return false;
        }

        int q = graphNodeToQubit(nodeId);
        if (q == -1) {
            std::cerr << "Node " << nodeId << " should have been activated, but is not present in graphNodeToQubit!\n";
            return false;
        }

        if (graph.isOutput(nodeId)) {
            writeOutAdjToStatevec(nodeId);
            readyToMeasure.erase(nodeId);
            return true;
        }

        auto [basis, angle] = graph.getMeasurement(nodeId);
        int outcome = backendSim.measure_qubit_in_basis(q, basis, angle);

        activeNodes.erase(nodeId);
        deactivatedNodes.insert(nodeId);
        tracedOutQubit(q);

        measured.insert(nodeId);
        measurementOutcomes[nodeId] = outcome;
        readyToMeasure.erase(nodeId);


        // Apply corrections following the strong uniform stepwise determinism
        // http://arxiv.org/abs/2410.23439 (An algebraic interpretation of PF) p. 5
        if (outcome == 1) {  // unwanted result
            if (flow.corrf.count(nodeId)) {
                for (int target : flow.corrf.at(nodeId)) {
                    if (isMeasured(target)) continue;
                    appliedCorrections[target].insert("X");  // add X to all in corrf
                    rotateGraphNode(target, "X");
                }
                for (int target : graph.oddNeighborhood(flow.corrf.at(nodeId))) {
                    if (isMeasured(target)) continue;
                    appliedCorrections[target].insert("Z");  //add Z to all odd nieghbors
                    rotateGraphNode(target, "Z");
                }
            }
        }

        // Update dependencies of other nodes
        for (auto& [node, deps] : Xdependencies) {
            deps.erase(nodeId);
        }
        for (auto& [node, deps] : Zdependencies) {
            deps.erase(nodeId);
        }

        // Recompute ready nodes
        recomputeReadyToMeasure(nodeId);

        if (conveyorBelt) activateAllNecessary();

        return true;
    }


    /// Applies output vertex `outId`'s accumulated OutputAdjustmentMap to its backend qubit as real gates (OutputAdjustmentMap::toCircuit()), then resets the tracked adjustment back to standard.
    void writeOutAdjToStatevec(int outId) {

        if (!graph.isOutput(outId)) {
            std::cerr << "Simulator write OutAdj: OutId is not an output!\n";
            return;
        }

        auto& oa = graph.getOutputAdjustment(outId);
        int q = graphNodeToQubit(outId);

        for (const auto& gate : oa.toCircuit().gates) {
            std::string op = gate.name;
            std::transform(op.begin(), op.end(), op.begin(), ::tolower);
            if (op == "h") {
                backendSim.H(q);
            } else if (op == "z") {
                backendSim.Z(q);
            } else if (op == "s") {
                backendSim.S(q);
            } else {
                std::cerr << "Unsupported gate from Output Adjustment: " << gate.name << "\n";
            }
        }
        oa.reset();
    }

    /// Reorders the backend's qubits so that the node with the lowest graph node ID sits at bit 0 (rightmost in the bitstring) and the highest node ID sits at the most significant bit (leftmost). Called after every activation so this canonical order holds at all times; skipped (O(2^n) cost) once the statevector has grown past `maxVecSizeJSON`.
    void reorderQubitsCanonically() {
        int n = (int)qubitToGraphNode.size();

        // Reordering costs O(2^n); skip it once the vector has grown past
        // the size we'd bother returning/displaying anyway.
        if ((1 << n) > maxVecSizeJSON) return;

        // Build a sorted list of (nodeId, currentQubitIndex)
        std::vector<std::pair<int,int>> nodeToQubit(n);
        for (int q = 0; q < n; ++q)
            nodeToQubit[q] = {qubitToGraphNode[q], q};

        // Sort by node ID ascending: lowest node ID should end up at bit 0
        std::sort(nodeToQubit.begin(), nodeToQubit.end());

        // permutation[new_qubit] = old_qubit
        // new qubit 0 (LSB) = the qubit currently holding the lowest node ID
        std::vector<int> permutation(n);
        for (int new_q = 0; new_q < n; ++new_q)
            permutation[new_q] = nodeToQubit[new_q].second;

        backendSim.reorderQubits(permutation);

        // Update qubitToGraphNode to reflect the new ordering
        for (int new_q = 0; new_q < n; ++new_q)
            qubitToGraphNode[new_q] = nodeToQubit[new_q].first;
    }


    /// Applies every output vertex's accumulated OutputAdjustmentMap (writeOutAdjToStatevec()) in turn.
    void writeAllOutAdjToStatevec() {
        for (auto [id, _] : graph.getOutputAdjustments()) {
            writeOutAdjToStatevec(id);
        }
    }

    /// Whether the simulation has finished: nothing left ready to measure, or every measurement vertex is measured and every output's adjustment is already standard.
    bool isComplete() const {

        if (readyToMeasure.size() == 0) return true;

        if (measured.size() >= graph.mvertices().size()) {
            for (auto i : readyToMeasure) {
                const auto oa = graph.getOutputAdjustment(i);
                if (!oa.isStandard()) return false;
            }
            return true;
        }

        return false;
    }


    /// Repeatedly step()s an arbitrary ready vertex until isComplete(). Stops early (with a diagnostic to stderr) if a step ever fails.
    void simulateAll() {
        while (!isComplete()) {
            bool succes = step(*readyToMeasure.begin());
            if (!succes) {
                std::cerr << "Interrupting simulateAll because of a unsuccesful step!\n";
                return;
            }
        }
    }

    /// Runs simulateAll() and returns the final state as a bra-ket string.
    std::string runAndGetOutput() {
        simulateAll();
        return getStatevectorBraKet();
    }

    /// The measurement outcome (0 or 1) recorded for each measured vertex so far.
    const std::unordered_map<int, int>& getOutcomes() const {
        return measurementOutcomes;
    }

    /// The set of correction types (`"X"`/`"Z"`) applied to each vertex so far.
    const std::unordered_map<int, std::set<std::string>>& getCorrections() const {
        return appliedCorrections;
    }

};

#endif // MBQC_SIMULATOR_HPP
