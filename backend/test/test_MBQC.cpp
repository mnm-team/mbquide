#include "doctest.h"
#include "test_helpers.hpp"
#include "utils.hpp"
#include "MBQC_Graph.hpp"
#include "ZX_Graph.hpp"
#include "ZX2MBQC.hpp"
#include "MBQC2ZX.hpp"
#include "QASM_Parser.hpp"
#include "Quantum_Circuit.hpp"
#include "Flow.hpp"
#include "test_main.cpp"
#include <cmath>
#include <random>


TEST_CASE("MBQC_Graph Initialization") {
    
    MBQC_Graph graph(3, {0, 1}, {2});

    CHECK(graph.getSize() == 3);
    CHECK(graph.getAdjacencyMatrix().size() == 3);
    CHECK(graph.getAdjacencyMatrix()[0][0] == 0);
    CHECK(graph.getAdjacencyMatrix()[1][2] == 0);
}

TEST_CASE("Add edges to MBQC_Graph") {
    
    MBQC_Graph graph(3, {0, 1}, {2});
    
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    const auto& adjMatrix = graph.getAdjacencyMatrix();
    CHECK(adjMatrix[0][1] == 1);
    CHECK(adjMatrix[1][2] == 1);
}

TEST_CASE("Set and get measurements") {
    MBQC_Graph graph(3, {0}, {2});
    
    graph.setMeasurement(0, MeasurementBasis::X, 2 * M_PI);
    graph.setMeasurement(1, MeasurementBasis::Z, M_PI);

    auto [basis0, angle0] = graph.getMeasurement(0);
    auto [basis1, angle1] = graph.getMeasurement(1);

    CHECK(basis0 == MeasurementBasis::X);
    CHECK(fAlmostEqual(angle0, 0));
    
    CHECK(basis1 == MeasurementBasis::Z);
    CHECK(fAlmostEqual(angle1, M_PI));
}

TEST_CASE("Measurement basis string conversion") {
    CHECK(basisToString(MeasurementBasis::X) == "X");
    CHECK(basisToString(MeasurementBasis::Y) == "Y");
    CHECK(basisToString(MeasurementBasis::Z) == "Z");
    CHECK(basisToString(MeasurementBasis::XY) == "XY");
    CHECK(basisToString(MeasurementBasis::YZ) == "YZ");
    CHECK(basisToString(MeasurementBasis::XZ) == "XZ");
}

TEST_CASE("ZX -> MBQC -> ZX") {
    ZXGraph zx;

    int inputId = zx.addSpider(SpiderType::INPUT);
    int zSpiderId = zx.addSpider(SpiderType::Z, 0);
    int outputId = zx.addSpider(SpiderType::OUTPUT);
    
    zx.addEdge(inputId, zSpiderId, EdgeType::SIMPLE);
    zx.addEdge(zSpiderId, outputId, EdgeType::HADAMARD);
    
    MBQC_Graph mbqc = ZXtoMBQCGraph(zx);

    ZXGraph newZX = MBQCtoZXGraph(mbqc);

    CHECK(compareTensors(zx, newZX));

}


TEST_CASE("QASM -> Circuit -> ZX -> MBQC -> ZX") {

    SUBCASE("Coin Toss Circuit") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            h q[0];
        )qasm";
    
        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
    
        CHECK(mbqc.getInputs().size() == 1);
        CHECK(mbqc.getSize() > 3);
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Bell State Circuit") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
            cx q[0],q[1];
        )qasm";
    
        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(mbqc.getInputs().size() == 2);
        CHECK(mbqc.getSize() > 5);
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Simple X Circuit") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            x q[0];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Simple X with two qubits") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            x q[0];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Simple H") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[1];
            h q[0];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Simple H with two qubits") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[0];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }

    SUBCASE("Simple H with two qubits 2") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[2];
            h q[1];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }

    // TODO: something with ordering not correct when comparing for circuits >= 3 qubits
    SUBCASE("Simple X with three qubits") {
        const char* qasm_text = R"qasm(
            OPENQASM 2.0;
            qreg q[3];
            x q[0];
        )qasm";

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();
        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = ZXtoMBQCGraph(originalZX);
        ZXGraph newZX = MBQCtoZXGraph(mbqc);
        
        CHECK(compareTensors(newZX, originalZX));
    }


}

TEST_CASE("Test Relabeling") {
    
    MBQC_Graph graph(4, {0}, {3});
    
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    SUBCASE("First Relabeling XY") {
        
    
        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, 0);
        graph.setMeasurement(1, MeasurementBasis::XY, M_PI/2);
        graph.setMeasurement(2, MeasurementBasis::YZ, M_PI/4);
    
        MBQC_Graph original = graph.clone();
        
        graph.relabel(1);
    
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }

    SUBCASE("Second Relabeling XZ PI/2") {

        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, 0);
        graph.setMeasurement(1, MeasurementBasis::XZ, M_PI/2);
        graph.setMeasurement(2, MeasurementBasis::YZ, M_PI/4);

        MBQC_Graph original = graph.clone();
        
        graph.relabel(1);

        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }

    SUBCASE("Third Relabeling XZ 0") {

        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, 0);
        graph.setMeasurement(1, MeasurementBasis::XZ, 0);
        graph.setMeasurement(2, MeasurementBasis::YZ, 4.1212421441);

        MBQC_Graph original = graph.clone();
        
        graph.relabel(1);

        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }
}


TEST_CASE("Test local complementation") {
    
    MBQC_Graph graph(4, {0}, {3});
    
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    SUBCASE("Simple example of two lc") {
    
        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, M_PI);
        graph.setMeasurement(1, MeasurementBasis::Y, 0);
        graph.setMeasurement(2, MeasurementBasis::Z, M_PI);
        
        MBQC_Graph original = graph.clone();
    
        // Perform local complementation on node 1
        graph.localComplementation(1);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    
        // Check if applying local complementation on the same node again is correct
        graph.localComplementation(1);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }


    SUBCASE("lc near output node") {  // Rz(-pi/2)
    
        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, M_PI);
        graph.setMeasurement(1, MeasurementBasis::Y, 0);
        graph.setMeasurement(2, MeasurementBasis::Z, M_PI);
        
        MBQC_Graph original = graph.clone();
    
        // Perform local complementation on node 2
        graph.localComplementation(2);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));

        // Check if applying local complementation on the same node again is correct
        graph.localComplementation(2);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }

    SUBCASE("lc on output node") {
    
        // Set initial measurements for nodes
        graph.setMeasurement(0, MeasurementBasis::X, M_PI);
        graph.setMeasurement(1, MeasurementBasis::Y, 0);
        graph.setMeasurement(2, MeasurementBasis::Z, M_PI);
        
        MBQC_Graph original = graph.clone();
    
        // Perform local complementation on node 3
        graph.localComplementation(3);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    
        // Check if applying local complementation on the same node again is correct
        graph.localComplementation(3);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }
    
    

    SUBCASE("Another example not working properly for angles that are not multiples of pi/4") {  // TODO

        // Working for Number of local complementations:
            // 0  ✅
            // 1  ❌
            // 2  ❌
            // 3  ✅
            // 4  ✅
            // 5  ❌
            // 6  ❌
            // 7  ✅
            // 8  ✅
            // 9  ❌
            // 10 ❌

        graph.setMeasurement(0, MeasurementBasis::YZ, 7 * M_PI / 4);
        graph.setMeasurement(1, MeasurementBasis::XZ, M_PI / 2);
        graph.setMeasurement(2, MeasurementBasis::XY, 3 * M_PI / 4);

        MBQC_Graph original = graph.clone();

        // Perform local complementation on node 1
        graph.localComplementation(1);
        
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }
}



TEST_CASE("Randomized local complementation") {
    std::mt19937 rng(42);
    int numVertices = 10;

    MBQC_Graph graph(numVertices, {0}, {numVertices-1});
    graph.addEdge(numVertices-2, numVertices-1);
    graph.addEdge(numVertices-3, numVertices-2);
    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(numVertices-2, MeasurementBasis::X);
    
    std::uniform_int_distribution<int> vertexDist(0, numVertices-3);
    for (int i = 0; i < numVertices * 2; ++i) {
        int u = vertexDist(rng);
        int v = vertexDist(rng);
        if (u != v) {
            graph.addEdge(u, v);
        }
    }

    std::array<MeasurementBasis, 3> bases = {
        MeasurementBasis::XY,
        MeasurementBasis::XZ,
        MeasurementBasis::YZ
    };
    std::uniform_int_distribution<size_t> basisDist(0, bases.size() - 1);
    // std::uniform_real_distribution<double> angleDist(0.0, 2 * M_PI);
    std::uniform_int_distribution<int> discreteAngleDist(0, 7);  // TODO: only works for the discrete distribution

    for (int v = 0; v < numVertices-2; ++v) {
        MeasurementBasis basis = bases[basisDist(rng)];
        double angle = discreteAngleDist(rng) * (M_PI / 4);
        graph.setMeasurement(v, basis, angle);
    }

    std::uniform_int_distribution<int> nodeDist(1, numVertices-1);
    std::vector<int> testNodes;
    for (int i = 0; i < 3; ++i) {
        testNodes.push_back(nodeDist(rng));
    }

    MBQC_Graph original = graph.clone();

    for (int n : testNodes) {
        graph.localComplementation(n);
    }

    CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
}


TEST_CASE("Test ZInsertion and ZDeletion") {

    SUBCASE("Basic") {
        
        MBQC_Graph graph(6, {0, 1}, {5});
    
        graph.addEdge(0, 2);
        graph.addEdge(1, 3);
        graph.addEdge(2, 3);
        graph.addEdge(3, 4);
        graph.addEdge(4, 5);
    
        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(1, MeasurementBasis::X);
        graph.setMeasurement(2, MeasurementBasis::XY, 2.03813);
        graph.setMeasurement(3, MeasurementBasis::YZ, 1.97273);
        graph.setMeasurement(4, MeasurementBasis::YZ, 1.97273);
    
        MBQC_Graph original = graph.clone();
    
        // Perform ZInsertion
        std::vector<int> vertices = {2, 3, 4};
        graph.ZInsertion(vertices);
    
        CHECK(graph.getSize() == 7);
        CHECK(graph.getAdjacencyMatrix().size() == 7);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    
        // Perform ZDeletion on node new node
        graph.ZDeletion(6);
    
        CHECK(graph.getSize() == 6);
        CHECK(graph.getAdjacencyMatrix().size() == 6);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));

    }

    SUBCASE("Z-Deletion next to output") {
        MBQC_Graph graph(5, {0}, {3});
    
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.addEdge(2, 3);
        
        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(1, MeasurementBasis::X);
        graph.setMeasurement(2, MeasurementBasis::X);
        
        // Z_Measurement that will be removed
        graph.setMeasurement(4, MeasurementBasis::YZ, M_PI);
        graph.addEdge(4, 0);
        graph.addEdge(4, 1);
        graph.addEdge(4, 2);
        graph.addEdge(4, 3);  // should update outputAdjustments
    
        MBQC_Graph original = graph.clone();
    
        // Perform ZDeletion
        graph.ZDeletion(4);
    
        CHECK(graph.getSize() == 4);
        CHECK(graph.getAdjacencyMatrix().size() == 4);
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));

    }
    
}



TEST_CASE("Test pivot operation (not connected to Output)") {
    
    MBQC_Graph graph(7, {0, 1}, {6});

    graph.addEdge(0, 2);
    graph.addEdge(1, 3);
    graph.addEdge(2, 3);  // pivot edge
    graph.addEdge(2, 4);
    graph.addEdge(3, 5);
    graph.addEdge(4, 5);
    graph.addEdge(5, 6);

    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::X);
    graph.setMeasurement(2, MeasurementBasis::XY, M_PI/2);
    graph.setMeasurement(3, MeasurementBasis::YZ, M_PI/4);
    graph.setMeasurement(4, MeasurementBasis::XZ, 3*M_PI/2);
    graph.setMeasurement(5, MeasurementBasis::XZ);

    MBQC_Graph original = graph.clone();

    graph.pivot(2, 3);

    CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));

}


TEST_CASE("Randomized pivot test") {
    std::mt19937 rng(42);
    
    std::vector<std::pair<int,int>> pivotableEdges;
    
    int numVertices = 15;

    MBQC_Graph graph(numVertices, {0, 1}, {numVertices-1});
    std::uniform_int_distribution<int> vertexDist(2, numVertices-4);
    graph.addEdge(numVertices-2, numVertices-1);
    graph.addEdge(numVertices-3, numVertices-2);
    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::X);
    graph.setMeasurement(numVertices-2, MeasurementBasis::X);

    pivotableEdges.push_back(std::make_pair(numVertices-2, numVertices-1));
    pivotableEdges.push_back(std::make_pair(numVertices-3, numVertices-2));


    for (int i = 0; i < 2 * numVertices; ++i) {
        int u = vertexDist(rng);
        int v = vertexDist(rng);
        if (u != v) graph.addEdge(u, v);
        pivotableEdges.push_back(std::make_pair(u, v));
    }

    std::array<MeasurementBasis, 3> bases = {
        MeasurementBasis::XY,
        MeasurementBasis::XZ,
        MeasurementBasis::YZ
    };

    std::uniform_int_distribution<size_t> basisDist(0, bases.size() - 1);
    std::uniform_real_distribution<double> angleDist(0.0, 2*M_PI);
    for (int v = 2; v < numVertices-2; ++v) {
        MeasurementBasis basis = bases[basisDist(rng)];
        double angle = angleDist(rng);
        graph.setMeasurement(v, basis, angle);
    }

    std::uniform_int_distribution<size_t> edgeDist(0, pivotableEdges.size()-1);
    auto pivotEdge = pivotableEdges[edgeDist(rng)];

    MBQC_Graph original = graph.clone();
    graph.pivot(pivotEdge.first, pivotEdge.second);

    CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
}


TEST_CASE("Test invalid graph operations") {

    MBQC_Graph graph(3, {0, 1}, {2});

    // Test invalid edge
    graph.addEdge(0, 3);
    CHECK(graph.getAdjacencyMatrix()[0][3] == 0);  // No edge should exist

    // Test invalid measurement
    std::cerr << "Ignore this setting node error - ";
    graph.setMeasurement(3, MeasurementBasis::Y, M_PI);
}


TEST_CASE("Get flow demand matrix") {
    MBQC_Graph graph(6, {0}, {5});

    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::Y);
    graph.setMeasurement(2, MeasurementBasis::XY);
    graph.setMeasurement(3, MeasurementBasis::YZ);
    graph.setMeasurement(4, MeasurementBasis::XZ);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 4);
    graph.addEdge(4, 5);

    auto flow = graph.getFlowDemandMatrix();
    std::vector<std::vector<int>> correct = {
        {1, 0, 0, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0}
    };

    CHECK(flow == correct);


    // Another example
    graph = MBQC_Graph(3, {0}, {2});

    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::Z);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    flow = graph.getFlowDemandMatrix();
    
    correct = {
        {1, 1},
        {1, 0}
    };
    
    CHECK(flow == correct);

}

TEST_CASE("Invert Matrix") {
    MBQC_Graph graph(3, {0}, {2});
    
    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::Z);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    auto flow = graph.getFlowDemandMatrix();    
    auto invertedFlow = getInverse(flow);

    std::vector<std::vector<double>> correct = {
        {0, 1},
        {1, -1}
    };

    CHECK(invertedFlow == correct);


    // Another example (not invertible)

    graph = MBQC_Graph(6, {0}, {5});

    graph.setMeasurement(0, MeasurementBasis::X);
    graph.setMeasurement(1, MeasurementBasis::Y);
    graph.setMeasurement(2, MeasurementBasis::XY);
    graph.setMeasurement(3, MeasurementBasis::YZ);
    graph.setMeasurement(4, MeasurementBasis::XZ);

    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 4);
    graph.addEdge(4, 5);

    flow = graph.getFlowDemandMatrix();
    invertedFlow = getInverse(flow);

    CHECK(invertedFlow.empty());
}



TEST_CASE("Find Pauli Flow") {

    SUBCASE("No flow should be found") {

        MBQC_Graph graph(6, {0}, {4,5});

        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(1, MeasurementBasis::Y);
        graph.setMeasurement(2, MeasurementBasis::XY);
        graph.setMeasurement(3, MeasurementBasis::YZ);

        // Run Pauli flow finder
        std::cerr << "Ignore this 'no flow found' Info - ";
        PauliFlowResult result = findPauliFlow(graph);

        CHECK(!result.ok);
        
        // Depth map must contain all vertices
        CHECK(result.depths.size() == 0);

    }

    SUBCASE("Example that has flow (according to graphix)") {

            const char* qasm_text = R"qasm(
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
creg c[2];

// Apply rotation gates
rz(3.141) q[0];
rz(3.141) q[1];
cx q[0], q[1];
s q[0];
cx q[1], q[0];
rz(3.141) q[1];
cx q[1], q[0];
rz(3.141) q[0];
            )qasm";
        
            QASMParser qasm = QASMParser("", qasm_text);
            QuantumCircuit circ = qasm.parse();
            ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
            MBQC_Graph g = ZXtoMBQCGraph(originalZX);
        
            // Run Pauli flow finder
            PauliFlowResult result = findPauliFlow(g);
        
            CHECK(result.ok);
            
            // Depth map must contain all vertices
            CHECK(result.depths.size() == g.getSize());

        
    }

}

TEST_CASE("Output Adjustment"){

    MBQC_Graph graph(4, {0}, {2});
        
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(3, 2);  // For z elimination

    graph.setMeasurement(0, MeasurementBasis::X, M_PI);
    graph.setMeasurement(1, MeasurementBasis::Y, 0);
    graph.setMeasurement(3, MeasurementBasis::XZ, M_PI);

    SUBCASE("Perform local complementation on out node") {  // Rx(pi/2)
        graph.localComplementation(2);

        auto outAdjs = graph.getOutputAdjustments();

        CHECK(outAdjs[2].toString() == "X: +X, Z: -Y");
    }

    SUBCASE("Perform local complementation on neighbor of out node") {  // Rz(-pi/2) | S†
        graph.localComplementation(1);

        auto outAdjs = graph.getOutputAdjustments();
        
        CHECK(outAdjs[2].toString() == "X: -Y, Z: +Z");
    }

    SUBCASE("Perform z elimination on neighbor of out") {  // Z
        graph.ZDeletion(3);

        auto outAdjs = graph.getOutputAdjustments();
        
        CHECK(outAdjs[2].toString() == "X: -X, Z: +Z");
    }

}


