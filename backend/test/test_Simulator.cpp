#include "doctest.h"
#include "utils.hpp"
#include "Simulator.hpp"
#include "Statevector.hpp"
#include "MBQC_Graph.hpp"
#include "Flow.hpp"
#include "ZX_Graph.hpp"
#include "ZX2MBQC.hpp"
#include "QASM_Parser.hpp"
#include "Quantum_Circuit.hpp"
#include <iostream>


bool equalUpToGlobalPhase(const std::string& a, const std::string& b) {
    if (StatevectorSimulator::isEqualUpToGlobalPhase(StatevectorSimulator::parseBraKet(a), StatevectorSimulator::parseBraKet(b))) 
        return true;
    std::cout << "Not even up to global phase equal vectors:\n\t" << a << " != " << b << "\n";
    return false;
}


// Helper function to create a simple linear graph for testing
MBQC_Graph createLinearGraph(int size) {
    MBQC_Graph graph(size, {0}, {size-1});
    
    for (int i = 0; i < size - 1; ++i) {
        graph.addEdge(i, i + 1);
    }
    
    for (int i = 0; i < size - 1; ++i) {
        graph.setMeasurement(i, MeasurementBasis::X, 0.0);
    }
    
    return graph;
}


TEST_CASE("Simulator initialization") {
    SUBCASE("Valid initialization with flow") {
        MBQC_Graph graph = createLinearGraph(4);
        PauliFlowResult flow = findPauliFlow(graph);
        
        CHECK_NOTHROW(Simulator(graph, flow));
    }
    
    SUBCASE("Initialization with input state string") {
        MBQC_Graph graph = createLinearGraph(4);
        PauliFlowResult flow = findPauliFlow(graph);
        
        CHECK_NOTHROW(Simulator(graph, flow, true, "(1)|0>"));
    }
}


TEST_CASE("Measurement step execution") {
    MBQC_Graph graph = createLinearGraph(4);
    PauliFlowResult flow = findPauliFlow(graph);
    
    Simulator sim(graph, flow, true);
    
    SUBCASE("Valid measurement step") {
        auto ready = sim.getReadyNodes();
        if (!ready.empty()) {
            int nodeToMeasure = *ready.begin();
            CHECK(sim.step(nodeToMeasure));
        }
    }
    
    SUBCASE("Cannot measure non-ready node") {
        // Try to measure a node that's definitely not ready
        // (assuming node exists but has dependencies)
        auto ready = sim.getReadyNodes();
        int notReady = -1;
        
        for (int i = 0; i < 4; ++i) {
            if (ready.find(i) == ready.end() && !graph.isOutput(i)) {
                notReady = i;
                break;
            }
        }
        
        if (notReady >= 0) {
            std::cerr << "Ignore this 'node not ready' Info - ";
            CHECK_FALSE(sim.step(notReady));
        }
    }
}

TEST_CASE("Measurement outcomes tracking") {
    MBQC_Graph graph = createLinearGraph(4);
    PauliFlowResult flow = findPauliFlow(graph);
    
    Simulator sim(graph, flow, true);
    
    SUBCASE("Outcomes are recorded") {
        auto ready = sim.getReadyNodes();
        if (!ready.empty()) {
            int node = *ready.begin();
            sim.step(node);
            
            auto outcomes = sim.getOutcomes();
            CHECK(outcomes.find(node) != outcomes.end());
        }
    }
    
    SUBCASE("Outcome values are binary") {
        auto ready = sim.getReadyNodes();
        if (!ready.empty()) {
            int node = *ready.begin();
            sim.step(node);
            
            auto outcomes = sim.getOutcomes();
            int outcome = outcomes.at(node);
            CHECK((outcome == 0 || outcome == 1));
        }
    }
}

TEST_CASE("Complete simulation execution") {
    MBQC_Graph graph = createLinearGraph(4);
    PauliFlowResult flow = findPauliFlow(graph);
    
    Simulator sim(graph, flow, true);
    
    SUBCASE("Simulation completes when all measured") {
        CHECK_FALSE(sim.isComplete());
        
        // Measure all ready nodes until complete
        int maxSteps = 10;
        int steps = 0;
        
        while (!sim.isComplete() && steps < maxSteps) {
            auto ready = sim.getReadyNodes();
            if (ready.empty()) break;
            int node = *ready.begin();
            sim.step(node);
            steps++;
        }
        
        CHECK(sim.isComplete());
        CHECK(sim.getOutcomes().size() == 3);
        CHECK(3 <= steps);
        CHECK(steps <= 4);
    }
}

TEST_CASE("Deterministic vs random measurements") {
    MBQC_Graph graph = createLinearGraph(4);
    PauliFlowResult flow = findPauliFlow(graph);
    
    SUBCASE("Random measurements enabled") {
        Simulator sim(graph, flow, true);
        auto ready = sim.getReadyNodes();
        
        if (!ready.empty()) {
            int node = *ready.begin();
            sim.step(node);
            
            auto outcomes = sim.getOutcomes();
            CHECK(outcomes.find(node) != outcomes.end());
        }
    }
    
    SUBCASE("Deterministic measurements") {
        Simulator sim(graph, flow, false);
        auto ready = sim.getReadyNodes();
        
        if (!ready.empty()) {
            int node = *ready.begin();
            sim.step(node);
            
            auto outcomes = sim.getOutcomes();
            CHECK(outcomes.find(node) != outcomes.end());
        }
    }
}


TEST_CASE("Two qubit graph") {
    MBQC_Graph graph(2, {0}, {1});
    graph.setMeasurement(0, MeasurementBasis::X, 0.0);
    graph.addEdge(0, 1);
    
    PauliFlowResult flow;
    flow.ok = true;
    
    Simulator sim(graph, flow, true);
    
    sim.step(0);
    
    CHECK(sim.isComplete());
    auto outcomes = sim.getOutcomes();
    CHECK((outcomes == std::unordered_map<int,int>{{0, 0}} || outcomes == std::unordered_map<int,int>{{0, 1}}));
}

TEST_CASE("Graph with multiple measurements") {
    MBQC_Graph graph(5, {0}, {4});
    
    // Create a more complex graph structure
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 4);
    
    graph.setMeasurement(0, MeasurementBasis::X, 0.0);
    graph.setMeasurement(1, MeasurementBasis::X, 0.0);
    graph.setMeasurement(2, MeasurementBasis::Y, M_PI);
    graph.setMeasurement(3, MeasurementBasis::XY, M_PI/2);
    
    PauliFlowResult flow;
    flow.ok = true;
    flow.corrf[0] = {1};
    flow.corrf[1] = {2};
    flow.corrf[2] = {3};
    flow.corrf[3] = {4};
    
    Simulator sim(graph, flow, true);
    
    SUBCASE("Can measure all nodes in sequence") {
        int measurements = 0;
        int maxSteps = 10;
        
        while (!sim.isComplete() && measurements < maxSteps) {
            auto ready = sim.getReadyNodes();
            if (ready.empty()) break;
            
            int node = *ready.begin();
            if (sim.step(node)) {
                measurements++;
            }
        }
        
        CHECK(4 <= measurements);
        CHECK(measurements <= 5);
    }
}

TEST_CASE("Measurement basis handling") {
    MBQC_Graph graph(3, {0}, {2});
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    
    PauliFlowResult flow;
    flow.ok = true;
    flow.corrf[1] = {2};
    
    SUBCASE("X basis measurement") {
        graph.setMeasurement(1, MeasurementBasis::X, 0.0);
        CHECK_NOTHROW(Simulator(graph, flow, true));
    }
    
    SUBCASE("Y basis measurement") {
        graph.setMeasurement(1, MeasurementBasis::Y, 0.0);
        CHECK_NOTHROW(Simulator(graph, flow, true));
    }
    
    SUBCASE("Z basis measurement") {
        graph.setMeasurement(1, MeasurementBasis::Z, 0.0);
        CHECK_NOTHROW(Simulator(graph, flow, true));
    }
    
    SUBCASE("XY plane measurement") {
        graph.setMeasurement(1, MeasurementBasis::XY, M_PI/3);
        CHECK_NOTHROW(Simulator(graph, flow, true));
    }
}

TEST_CASE("Edge cases") {
    SUBCASE("Empty dependencies") {
        MBQC_Graph graph(3, {0}, {2});
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.setMeasurement(1, MeasurementBasis::X, 0.0);
        
        PauliFlowResult flow;
        flow.ok = true;
        // No dependencies - node 1 should be immediately ready
        
        Simulator sim(graph, flow, true);
        CHECK(sim.isReady(1));
    }

}


TEST_CASE("Bell state circuit test") {
    MBQC_Graph graph(7, {0,1}, {5,6});
    
    graph.setMeasurement(0, MeasurementBasis::X, 0.0);
    graph.setMeasurement(1, MeasurementBasis::X, 0.0);
    graph.setMeasurement(2, MeasurementBasis::X, 0.0);
    graph.setMeasurement(3, MeasurementBasis::X, 0.0);
    graph.setMeasurement(4, MeasurementBasis::X, 0.0);

    graph.addEdge(0, 2);
    graph.addEdge(1, 3);
    graph.addEdge(2, 3);
    graph.addEdge(2, 5);
    graph.addEdge(3, 4);
    graph.addEdge(4, 6);

    PauliFlowResult flow = findPauliFlow(graph);

    CHECK(flow.ok);

    SUBCASE("Input |00>") {
        Simulator sim(graph, flow, true);
    
        sim.simulateAll();

        CHECK(sim.isComplete());
    
        CHECK(equalUpToGlobalPhase(sim.getStatevectorBraKet(), "(0.707107)|00> + (0.707107)|11>"));
    }


    SUBCASE("Input |Bell>") {
        Simulator sim(graph, flow, true, "(0.707107)|00> + (0.707107)|01>");
    
        sim.simulateAll();
    
        CHECK(sim.isComplete());
    
        CHECK(equalUpToGlobalPhase(sim.getStatevectorBraKet(), "(1)|00>"));
    }

}


// Entire Pipeline:
// =============================================

std::string simulateCircuit(const char* qasm_text, const char* inputState) {
    QASMParser qasm = QASMParser("", qasm_text);
    QuantumCircuit circ = qasm.parse();
    ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
    MBQC_Graph graph = ZXtoMBQCGraph(originalZX);
    PauliFlowResult flow = findPauliFlow(graph);
    Simulator sim(graph, flow, true, inputState);
    return sim.runAndGetOutput();
}


TEST_CASE("Test Entire Pipeline (Simulating QASM Circuit)") {

    SUBCASE("Single Qubit Hadamard") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            h q[0];
        )qasm";

        const char* input = "(1)|0>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|0> + (0.707107)|1>"));
    }

    SUBCASE("Hadamard Twice Identity") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            h q[0];
            h q[0];
        )qasm";

        const char* input = "(1)|0>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|0>"));
    }

    SUBCASE("Pauli-X Gate") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            x q[0];
        )qasm";

        const char* input = "(1)|0>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|1>"));
    }

    SUBCASE("Pauli-X Twice Identity") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            x q[0];
            x q[0];
        )qasm";

        const char* input = "(1)|1>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|1>"));
    }

    SUBCASE("Z Gate Phase Flip") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            z q[0];
        )qasm";

        const char* input = "(1)|1>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(-1)|1>"));
    }

    SUBCASE("Z Gate On |0>") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            z q[0];
        )qasm";

        const char* input = "(1)|0>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|0>"));
    }

    SUBCASE("Simple CNOT No Trigger") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            cx q[0],q[1];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|00>"));
    }

    SUBCASE("Simple CNOT Triggered") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            cx q[0],q[1];
        )qasm";

        const char* input = "(1)|01>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|11>"));
    }

    SUBCASE("Bell State Creation From |00>") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            cx q[0],q[1];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|00> + (0.707107)|11>"));
    }

    SUBCASE("Bell State from |10> with X-gate") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            x q[1];
            h q[0];
            cx q[0],q[1];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|01> + (0.707107)|10>"));
    }

    SUBCASE("Three Qubit GHZ State") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[3];
            h q[0];
            cx q[0],q[1];
            cx q[1],q[2];
        )qasm";

        const char* input = "(1)|000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|000> + (0.707107)|111>"));
    }

    SUBCASE("Swap Using Three CNOTs") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            cx q[0],q[1];
            cx q[1],q[0];
            cx q[0],q[1];
        )qasm";

        const char* input = "(1)|10>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|01>"));
    }

    SUBCASE("Hadamard On Both Qubits") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            h q[1];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.5)|00> + (0.5)|01> + (0.5)|10> + (0.5)|11>"));
    }

    SUBCASE("Entangle Then Disentangle") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            cx q[0],q[1];
            cx q[0],q[1];
            h q[0];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|00>"));
    }


    SUBCASE("Four Qubit GHZ State") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[4];
            h q[0];
            cx q[0],q[1];
            cx q[1],q[2];
            cx q[2],q[3];
        )qasm";

        const char* input = "(1)|0000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|0000> + (0.707107)|1111>"));
    }

    SUBCASE("Three Qubit Uniform Superposition") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[3];
            h q[0];
            h q[1];
            h q[2];
        )qasm";

        const char* input = "(1)|000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output,
            "(0.353553)|000> + (0.353553)|001> + (0.353553)|010> + (0.353553)|011> + "
            "(0.353553)|100> + (0.353553)|101> + (0.353553)|110> + (0.353553)|111>"));
    }

    SUBCASE("Bell Pair With Extra Qubit") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[3];
            h q[1];
            cx q[1],q[2];
        )qasm";

        const char* input = "(1)|000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|000> + (0.707107)|110>"));
    }

    SUBCASE("Bell Pair With 2 Extra Qubits") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[4];
            h q[1];
            cx q[1],q[0];
        )qasm";

        const char* input = "(1)|0000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|0000> + (0.707107)|0011>"));
    }

    SUBCASE("Cascade CNOT Chain") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[4];
            cx q[0],q[1];
            cx q[1],q[2];
            cx q[2],q[3];
        )qasm";

        const char* input = "(1)|1000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|1111>"));
    }

    SUBCASE("Hadamard Entangle Then Flip Target") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            cx q[0],q[1];
            x q[1];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|01> + (0.707107)|10>"));
    }


    SUBCASE("X on 2 out of 4 Qubits") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[4];
            x q[0];
            x q[1];
        )qasm";

        const char* input = "(1)|0000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output,"(1)|0011>"));
    }

    SUBCASE("Double Bell Preparation On Four Qubits") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[4];
            h q[0];
            cx q[0],q[1];
            h q[2];
            cx q[2],q[3];
        )qasm";

        const char* input = "(1)|0000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output,
            "(0.5)|0000> + (0.5)|0101> + (0.5)|1010> + (0.5)|1111>"));  // Here the order that gets outputted of the simulator is still weird (it depends on the insertion order (via the id of the nodes))
    }

    SUBCASE("Three Qubit GHZ Then Undo") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[3];
            h q[0];
            cx q[0],q[1];
            cx q[1],q[2];
            cx q[1],q[2];
            cx q[0],q[1];
            h q[0];
        )qasm";

        const char* input = "(1)|000>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(1)|000>"));
    }

    SUBCASE("Mixed H X Z Circuit") {
        const char* circuit = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            x q[1];
            cx q[0],q[1];
            z q[0];
        )qasm";

        const char* input = "(1)|00>";

        auto output = simulateCircuit(circuit, input);
        CHECK(equalUpToGlobalPhase(output, "(0.707107)|01> + (-0.707107)|10>"));
    }

}