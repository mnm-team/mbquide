#include "doctest.h"
#include "test_helpers.hpp"
#include "utils.hpp"
#include "MBQC_Graph.hpp"
#include "Quantum_Circuit.hpp"
#include "QASM_Parser.hpp"
#include "Flow.hpp"
#include "Circ2MBQC.hpp"



TEST_CASE("Find Pauli Flow") {

    SUBCASE("No flow should be found") {

        MBQC_Graph graph(6, {0}, {4,5});

        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(1, MeasurementBasis::Y);
        graph.setMeasurement(2, MeasurementBasis::XY);
        graph.setMeasurement(3, MeasurementBasis::YZ);

        // Run Pauli flow finder
        PauliFlowResult result = findPauliFlow(graph);

        CHECK(!result.ok);
        
        // Depth map must contain all vertices
        CHECK(result.depths.size() == 0);

    }

    SUBCASE("Example that has flow") {

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
            MBQC_Graph g = CIRCtoMBQCGraph(circ);
        
            // Run Pauli flow finder
            PauliFlowResult result = findPauliFlow(g);
        
            CHECK(result.ok);
            
            // Depth map must contain all vertices
            CHECK(result.depths.size() == g.getSize());

        
    }

}

TEST_CASE("Pauli Flow - Simple chain") {

    MBQC_Graph g(3, {0}, {2});

    g.addEdge(0,1);
    g.addEdge(1,2);

    g.setMeasurement(0, MeasurementBasis::XY, M_PI/4);
    g.setMeasurement(1, MeasurementBasis::XY, M_PI/4);

    PauliFlowResult result = findPauliFlow(g);

    CHECK(result.ok);

    CHECK(checkPauliFlow(g, result));
    CHECK(checkFocussedFlow(g, result));
}

TEST_CASE("Pauli Flow - Star graph without flow") {

    MBQC_Graph g(5, {0}, {4});

    g.addEdge(0,1);
    g.addEdge(1,2);
    g.addEdge(1,3);
    g.addEdge(3,4);

    g.setMeasurement(0, MeasurementBasis::XY, M_PI/4);
    g.setMeasurement(1, MeasurementBasis::XY, M_PI/4);
    g.setMeasurement(2, MeasurementBasis::XZ, M_PI/4);
    g.setMeasurement(3, MeasurementBasis::YZ, M_PI/4);

    PauliFlowResult result = findPauliFlow(g);

    CHECK(!result.ok);
}

TEST_CASE("Pauli Flow - Pauli basis chain") {

    MBQC_Graph g(4, {0}, {3});

    g.addEdge(0,1);
    g.addEdge(1,2);
    g.addEdge(2,3);

    g.setMeasurement(0, MeasurementBasis::X);
    g.setMeasurement(1, MeasurementBasis::Y);
    g.setMeasurement(2, MeasurementBasis::X);

    PauliFlowResult result = findPauliFlow(g);

    CHECK(result.ok);

    CHECK(checkPauliFlow(g, result));
    CHECK(checkFocussedFlow(g, result));
}

TEST_CASE("Pauli Flow - 2D cluster fragment") {

    MBQC_Graph g(6, {0,1}, {4,5});

    g.addEdge(0,2);
    g.addEdge(1,3);
    g.addEdge(2,3);
    g.addEdge(2,4);
    g.addEdge(3,5);

    g.setMeasurement(0, MeasurementBasis::XY, M_PI/4);
    g.setMeasurement(1, MeasurementBasis::XY, M_PI/4);
    g.setMeasurement(2, MeasurementBasis::XY, M_PI/3);
    g.setMeasurement(3, MeasurementBasis::XY, M_PI/5);

    PauliFlowResult result = findPauliFlow(g);

    CHECK(result.ok);

    CHECK(checkPauliFlow(g, result));
    CHECK(checkFocussedFlow(g, result));
}


TEST_CASE("Pauli Flow - QASM converted circuit") {

    const char* qasm_text = R"qasm(
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];

h q[0];
cx q[0], q[1];
rz(pi/4) q[1];
cx q[1], q[0];
    )qasm";

    QASMParser qasm("", qasm_text);

    QuantumCircuit circ = qasm.parse();

    MBQC_Graph g = CIRCtoMBQCGraph(circ);

    PauliFlowResult result = findPauliFlow(g);

    CHECK(result.ok);

    CHECK(checkPauliFlow(g, result));
    CHECK(checkFocussedFlow(g, result));
}


TEST_CASE("Pauli Flow from QASM after simplification") {

    SUBCASE("Single qubit rotation chain") {

        const char* qasm_text = R"qasm(
OPENQASM 2.0;
include "qelib1.inc";

qreg q[1];

h q[0];
rz(pi/2) q[0];
rx(pi/2) q[0];
rz(pi/4) q[0];
h q[0];
        )qasm";

        QASMParser qasm("", qasm_text);

        QuantumCircuit circ = qasm.parse();

        MBQC_Graph g = CIRCtoMBQCGraph(circ);

        auto before = findPauliFlow(g);

        CHECK(before.ok);
        CHECK(checkPauliFlow(g, before));

        g.simplify();

        auto after = findPauliFlow(g);

        CHECK(after.ok);

        CHECK(checkPauliFlow(g, after));
        CHECK(checkFocussedFlow(g, after));
    }

    SUBCASE("Two qubit entangling circuit") {

        const char* qasm_text = R"qasm(
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];

h q[0];
h q[1];

cx q[0], q[1];

rz(pi/2) q[0];
rx(pi/2) q[1];

cx q[1], q[0];

rz(pi/4) q[1];
        )qasm";

        QASMParser qasm("", qasm_text);

        QuantumCircuit circ = qasm.parse();

        MBQC_Graph g = CIRCtoMBQCGraph(circ);

        auto before = findPauliFlow(g);

        CHECK(before.ok);
        CHECK(checkPauliFlow(g, before));

        g.simplify();

        auto after = findPauliFlow(g);

        CHECK(after.ok);

        CHECK(checkPauliFlow(g, after));
        CHECK(checkFocussedFlow(g, after));
    }

    SUBCASE("Three qubit Clifford-heavy circuit") {

        const char* qasm_text = R"qasm(
OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];

h q[0];
h q[1];
h q[2];

cx q[0], q[1];
cx q[1], q[2];

s q[0];
s q[1];
s q[2];

cx q[2], q[0];

rz(pi/2) q[1];
rx(pi/2) q[2];

cx q[0], q[2];

h q[1];
        )qasm";

        QASMParser qasm("", qasm_text);

        QuantumCircuit circ = qasm.parse();

        MBQC_Graph g = CIRCtoMBQCGraph(circ);

        auto before = findPauliFlow(g);

        CHECK(before.ok);
        CHECK(checkPauliFlow(g, before));

        g.simplify();

        auto after = findPauliFlow(g);

        CHECK(after.ok);

        CHECK(checkPauliFlow(g, after));
        CHECK(checkFocussedFlow(g, after));
    }
}