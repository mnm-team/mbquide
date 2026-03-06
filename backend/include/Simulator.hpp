#ifndef MBQC_SIMULATOR_HPP
#define MBQC_SIMULATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <iostream>
#include <json.hpp>
using json = nlohmann::json;

#include "MBQC_Graph.hpp"
#include "Flow.hpp"
#include "utils.hpp"
#include "Statevector.hpp"


class Simulator {

private:
    MBQC_Graph graph;
    PauliFlowResult flow;
    bool randomMeasurements;
    int totalNodes;
    int numInputNodes;
    std::string inputStateString;
    StatevectorSimulator statevectorSimulator;

    std::unordered_map<int, int> measurementOutcomes;
    std::unordered_map<int, std::set<std::string>> appliedCorrections;
    std::unordered_set<int> measured;
    std::unordered_set<int> activeNodes;
    std::unordered_set<int> deactivatedNodes;
    std::set<std::pair<int, int>> activeEdges;
    std::unordered_map<int, std::unordered_set<int>> dependencies;
    std::set<int> readyToMeasure;

    std::vector<int> qubitToGraphNode;
    int graphNodeToQubit(int n) {
        auto it = std::find(qubitToGraphNode.begin(), qubitToGraphNode.end(), n);
        if (it != qubitToGraphNode.end()) {
            int q = it - qubitToGraphNode.begin();
            return q;
        }
        return -1;  // means the qubit for this node does not exist
    }

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

    void rotateGraphNode(int u, std::string axis) {
        if (axis == "Z") {
            rotateGraphNodeZ(u);
        } else if (axis == "X") {
            rotateGraphNodeX(u);
        } else {
            throw std::runtime_error("Not implemented other graph node rotations than X and Z");
        }
    }
    
    
public:
    Simulator() = default;
    Simulator(const MBQC_Graph& g, const PauliFlowResult& flow, bool random = true, std::string inputState = "")
        : graph(g.clone()), flow(flow), randomMeasurements(random), inputStateString(inputState)
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
                dependencies[dep].insert(node);
            }
        }

        // Also include reverse dependencies from oddNCorrf
        for (const auto& [node, deps] : flow.oddNcorrf) {
            for (int dep : deps) {
                dependencies[dep].insert(node);
            }
        }
        
        for (int node = 0; node < totalNodes; ++node) {
            if (dependencies[node].empty() ||
                (dependencies[node].size() == 1 && dependencies[node].count(node) == 1)) {  // dependency is only the node itself
                readyToMeasure.insert(node);
            }
        }

        activateAllNecessary();

    }

    std::string getStatevectorBraKet() const {
        return statevectorSimulator.getStatevectorBraKet();
    }

    StatevectorSimulator getStatevectorSimulator() const {
        return statevectorSimulator;
    }

    json toJson() const {
        json j;

        j["graph"] = graph.toJson();
        j["flow"] = PauliFlowResultToJson(flow);

        
        j["readyToMeasure"] = readyToMeasure;
        j["measured"] = measured;
        j["outcomes"] = measurementOutcomes;
        j["statevector"] = statevectorSimulator.toJson();
        j["activeEdges"] = activeEdges;
        
        // The reverse of qubitToGraph has already the right order of statevecotr
        std::vector<int> active = qubitToGraphNode;
        std::reverse(active.begin(), active.end());
        j["activeNodes"] = active;

        return j;
    }

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

    bool isReady(int nodeId) const {
        return readyToMeasure.count(nodeId) > 0;
    }

    std::set<int> getReadyNodes() const {
        return readyToMeasure;
    }

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

    bool isMeasured(int nodeId) const {
        return measured.count(nodeId) > 0;
    }

    bool isActive(int nodeId) const {
        return activeNodes.count(nodeId) > 0;
    }

    bool wasDeactivated(int nodeId) const {
        return deactivatedNodes.count(nodeId) > 0;
    }

    bool isEdgeActive(int u, int v) const {
        return activeEdges.find({u, v}) != activeEdges.end() || activeEdges.find({v, u}) != activeEdges.end();
    }

    void activateNode(int nodeId) {
        if (isActive(nodeId)) return;
        if (wasDeactivated(nodeId)) return;
        int q = statevectorSimulator.add_qubit_plus();
        if (q != qubitToGraphNode.size()) std::cerr << "Activated new node but ID is not correct!\n";
        qubitToGraphNode.insert(qubitToGraphNode.begin(), nodeId);
        activeNodes.insert(nodeId);
    }

    void activateEdge(int u, int v) {
        if (isEdgeActive(u, v)) return;
        statevectorSimulator.CZ(graphNodeToQubit(u), graphNodeToQubit(v));
        activeEdges.insert({u,v});
    }

    // Activates all necessary nodes based on the readyToMEasure
    void activateAllNecessary() {
        for (int r : readyToMeasure) {
            for (int n : graph.getNeighbors(r)) {
                activateNode(n);
                activateEdge(r, n);
            }
        }
    }

    void initStatevector(std::string inputStateString = "") {
        statevectorSimulator = StatevectorSimulator(numInputNodes, randomMeasurements);

        if (!inputStateString.empty()) {
            auto inputState = StatevectorSimulator::parseBraKet(inputStateString);
            statevectorSimulator.setState(inputState);
        }

        std::vector<int> inputs = graph.getInputs();
        std::sort(inputs.begin(), inputs.end());
        
        qubitToGraphNode.reserve(numInputNodes);
        for (int i : inputs) {
            qubitToGraphNode.insert(qubitToGraphNode.begin(), i);
            activeNodes.insert(i);
        }
    }

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
        int outcome = statevectorSimulator.measure_qubit_in_basis(q, basis, angle);

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
        for (auto& [node, deps] : dependencies) {
            deps.erase(nodeId);
        }

        // Recompute ready nodes
        for (int node = 0; node < totalNodes; ++node) {
            if (graph.isInput(node) || isDone(node)) {
                continue;
            }
            if (dependencies[node].empty() ||
                (dependencies[node].size() == 1 && dependencies[node].count(node) == 1)) {  // dependency is only the node itself
                readyToMeasure.insert(node);
            }
        }

        activateAllNecessary();

        return true;
    }


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
                statevectorSimulator.H(q);
            } else if (op == "z") {
                statevectorSimulator.Z(q);
            } else if (op == "s") {
                statevectorSimulator.S(q);
            } else {
                std::cerr << "Unsupported gate from Output Adjustment: " << gate.name << "\n";
            }
        }
        oa.reset();
    }


    void writeAllOutAdjToStatevec() {
        for (auto [id, _] : graph.getOutputAdjustments()) {
            writeOutAdjToStatevec(id);
        }
    }

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


    void simulateAll() {
        while (!isComplete()) {
            bool succes = step(*readyToMeasure.begin());
            if (!succes) {
                std::cerr << "Interrupting simulateAll because of a unsuccesful step!\n";
                return;
            }
        }
    }

    std::string runAndGetOutput() {
        simulateAll();
        return getStatevectorBraKet();
    }

    const std::unordered_map<int, int>& getOutcomes() const {
        return measurementOutcomes;
    }
    
    const std::unordered_map<int, std::set<std::string>>& getCorrections() const {
        return appliedCorrections;
    }

};

#endif // MBQC_SIMULATOR_HPP
