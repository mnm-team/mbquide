#include "doctest.h"
#include "test_helpers.hpp"
#include "utils.hpp"
#include "MBQC_Graph.hpp"
#include "ZX_Graph.hpp"
#include "MBQC2ZX.hpp"
#include "Circ2MBQC.hpp"
#include "QASM_Parser.hpp"
#include "Quantum_Circuit.hpp"
#include <array>
#include <cmath>
#include <random>


namespace {

int countEdges(const MBQC_Graph& g) {
    return static_cast<int>(g.getAllEdges().size()) / 2;
}

struct SimplificationCircuit {
    const char* name;
    const char* qasm;
    // Whether greedyOptimizeEdges() is expected to strictly reduce the edge count of the
    // as-built (not yet simplified) MBQC graph for this circuit. False for circuits whose
    // graph has no exploitable local structure at all (e.g. a single edge).
    bool expectReduction;
};

const SimplificationCircuit kCircuits[] = {
    {"Coin Toss Circuit", R"qasm(
        OPENQASM 2.0;
        qreg q[1];
        h q[0];
    )qasm", false},
    {"Bell State Circuit", R"qasm(
        OPENQASM 2.0;
        qreg q[2];
        h q[0];
        cx q[0],q[1];
    )qasm", true},
    {"Simple X Circuit", R"qasm(
        OPENQASM 2.0;
        qreg q[1];
        x q[0];
    )qasm", false},
    {"Simple H with two qubits", R"qasm(
        OPENQASM 2.0;
        qreg q[2];
        h q[1];
    )qasm", true},
    {"Simple X with three qubits", R"qasm(
        OPENQASM 2.0;
        qreg q[3];
        x q[0];
    )qasm", true},
    {"GHZ-like three qubit circuit", R"qasm(
        OPENQASM 2.0;
        qreg q[3];
        h q[0];
        cx q[0],q[1];
        cx q[1],q[2];
        t q[2];
    )qasm", true},
};

} // namespace


TEST_CASE("simplify() preserves the represented tensor") {

    for (const auto& c : kCircuits) {
        SUBCASE(c.name) {
            QASMParser qasm = QASMParser("", c.qasm);
            QuantumCircuit circ = qasm.parse();

            ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
            MBQC_Graph mbqc = CIRCtoMBQCGraph(circ);
            mbqc.simplify();
            ZXGraph newZX = MBQCtoZXGraph(mbqc);

            CHECK(compareTensors(newZX, originalZX));
        }
    }

    SUBCASE("Random Clifford Circuit - 2 qubits") {
        std::string qasm_text = randomClifford(2, 5);

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();

        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = CIRCtoMBQCGraph(circ);
        mbqc.simplify();
        ZXGraph newZX = MBQCtoZXGraph(mbqc);

        CHECK(compareTensors(newZX, originalZX));
    }
}


TEST_CASE("greedyOptimizeEdges() preserves the represented tensor") {

    for (const auto& c : kCircuits) {
        SUBCASE(c.name) {
            QASMParser qasm = QASMParser("", c.qasm);
            QuantumCircuit circ = qasm.parse();

            ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
            MBQC_Graph mbqc = CIRCtoMBQCGraph(circ);

            int edgesBefore = countEdges(mbqc);
            mbqc.greedyOptimizeEdges();
            int edgesAfter = countEdges(mbqc);

            ZXGraph newZX = MBQCtoZXGraph(mbqc);

            CHECK(compareTensors(newZX, originalZX));
            if (c.expectReduction) {
                CHECK(edgesAfter < edgesBefore);
            } else {
                CHECK(edgesAfter <= edgesBefore);
            }

            // A fresh pass over an already-optimized graph should find nothing left to apply.
            CHECK(mbqc.greedyOptimizeEdges().empty());
        }
    }

    SUBCASE("Random Clifford Circuit - 2 qubits") {
        std::string qasm_text = randomClifford(2, 5);

        QASMParser qasm = QASMParser("", qasm_text);
        QuantumCircuit circ = qasm.parse();

        ZXGraph originalZX = ZXGraph::fromQuantumCircuit(circ);
        MBQC_Graph mbqc = CIRCtoMBQCGraph(circ);

        int edgesBefore = countEdges(mbqc);
        mbqc.greedyOptimizeEdges();
        int edgesAfter = countEdges(mbqc);

        ZXGraph newZX = MBQCtoZXGraph(mbqc);

        CHECK(compareTensors(newZX, originalZX));
        CHECK(edgesAfter <= edgesBefore);
    }
}


TEST_CASE("greedyOptimizeEdges() relabels Clifford-angle planar nodes before optimizing") {
    // Regression test: CIRCtoMBQCGraph builds every non-output node as XY-basis, even when its
    // angle is a Clifford multiple of pi/2 (semantically Pauli X or Y). Without relabeling those
    // to Pauli basis first, lcompCost/pivotCost's X/Y-basis z_bonus never triggers and
    // greedyOptimizeEdges finds nothing to do, even on graphs with obviously redundant
    // structure like this plain chain.
    MBQC_Graph graph(4, {0}, {3});
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);

    graph.setMeasurement(0, MeasurementBasis::XY, 0);         // semantically X
    graph.setMeasurement(1, MeasurementBasis::XY, M_PI / 2);  // semantically Y
    graph.setMeasurement(2, MeasurementBasis::XY, 0);         // semantically X

    MBQC_Graph original = graph.clone();

    int edgesBefore = countEdges(graph);
    auto rules = graph.greedyOptimizeEdges();
    int edgesAfter = countEdges(graph);

    CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    CHECK_FALSE(rules.empty());
    CHECK(edgesAfter < edgesBefore);
}


TEST_CASE("canOptimizeEdges() reports whether greedyOptimizeEdges() would do anything, without mutating the graph") {

    SUBCASE("Reports true and leaves the graph untouched when a rewrite is available") {
        MBQC_Graph graph(4, {0}, {3});
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.addEdge(2, 3);

        graph.setMeasurement(0, MeasurementBasis::XY, 0);
        graph.setMeasurement(1, MeasurementBasis::XY, M_PI / 2);
        graph.setMeasurement(2, MeasurementBasis::XY, 0);

        MBQC_Graph original = graph.clone();

        CHECK(graph.canOptimizeEdges());

        // The check must not have mutated the graph.
        CHECK(graph.getSize() == original.getSize());
        CHECK(countEdges(graph) == countEdges(original));
        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));

        // And it must agree with actually running the optimizer.
        CHECK_FALSE(graph.greedyOptimizeEdges().empty());
    }

    SUBCASE("Reports false on a graph with no exploitable structure") {
        // A single edge has no neighbor-of-neighbor structure for LC or pivot to exploit.
        MBQC_Graph graph(2, {0}, {1});
        graph.addEdge(0, 1);
        graph.setMeasurement(0, MeasurementBasis::XY, M_PI / 4);

        CHECK_FALSE(graph.canOptimizeEdges());
        CHECK(graph.greedyOptimizeEdges().empty());
    }

    SUBCASE("Reports false once a graph has already been optimized to its fixed point") {
        MBQC_Graph graph(4, {0}, {3});
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.addEdge(2, 3);

        graph.setMeasurement(0, MeasurementBasis::XY, 0);
        graph.setMeasurement(1, MeasurementBasis::XY, M_PI / 2);
        graph.setMeasurement(2, MeasurementBasis::XY, 0);

        REQUIRE(graph.canOptimizeEdges());
        graph.greedyOptimizeEdges();

        CHECK_FALSE(graph.canOptimizeEdges());
    }
}


TEST_CASE("greedyOptimizeEdges() preserves the tensor on hand-built graphs") {

    SUBCASE("Chain with a Y node (triggers lcomp + Z-deletion)") {
        MBQC_Graph graph(4, {0}, {3});
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.addEdge(2, 3);

        graph.setMeasurement(0, MeasurementBasis::X, M_PI);
        graph.setMeasurement(1, MeasurementBasis::Y, 0);
        graph.setMeasurement(2, MeasurementBasis::Z, M_PI);

        MBQC_Graph original = graph.clone();
        graph.greedyOptimizeEdges();

        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
    }

    SUBCASE("Denser graph with mixed planar bases (triggers pivot + nu-sets)") {
        MBQC_Graph graph(7, {0, 1}, {6});
        graph.addEdge(0, 2);
        graph.addEdge(1, 3);
        graph.addEdge(2, 3);
        graph.addEdge(2, 4);
        graph.addEdge(3, 5);
        graph.addEdge(4, 5);
        graph.addEdge(5, 6);

        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(1, MeasurementBasis::X);
        graph.setMeasurement(2, MeasurementBasis::XY, M_PI / 2);
        graph.setMeasurement(3, MeasurementBasis::YZ, M_PI / 4);
        graph.setMeasurement(4, MeasurementBasis::XZ, 3 * M_PI / 2);
        graph.setMeasurement(5, MeasurementBasis::XZ);

        MBQC_Graph original = graph.clone();

        int edgesBefore = countEdges(graph);
        graph.greedyOptimizeEdges();
        int edgesAfter = countEdges(graph);

        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
        CHECK(edgesAfter < edgesBefore);
        CHECK(graph.greedyOptimizeEdges().empty());
    }

    SUBCASE("Randomized graph") {
        std::mt19937 rng(7);
        int numVertices = 10;

        MBQC_Graph graph(numVertices, {0}, {numVertices - 1});
        graph.addEdge(numVertices - 2, numVertices - 1);
        graph.addEdge(numVertices - 3, numVertices - 2);
        graph.setMeasurement(0, MeasurementBasis::X);
        graph.setMeasurement(numVertices - 2, MeasurementBasis::X);

        std::uniform_int_distribution<int> vertexDist(0, numVertices - 3);
        for (int i = 0; i < numVertices * 2; ++i) {
            int u = vertexDist(rng);
            int v = vertexDist(rng);
            if (u != v) graph.addEdge(u, v);
        }

        std::array<MeasurementBasis, 4> bases = {
            MeasurementBasis::XY,
            MeasurementBasis::XZ,
            MeasurementBasis::YZ,
            MeasurementBasis::Y
        };
        std::uniform_int_distribution<size_t> basisDist(0, bases.size() - 1);
        std::uniform_int_distribution<int> discreteAngleDist(0, 7);

        for (int v = 0; v < numVertices - 2; ++v) {
            MeasurementBasis basis = bases[basisDist(rng)];
            double angle = (basis == MeasurementBasis::Y) ? 0.0 : discreteAngleDist(rng) * (M_PI / 4);
            graph.setMeasurement(v, basis, angle);
        }

        MBQC_Graph original = graph.clone();

        int edgesBefore = countEdges(graph);
        graph.greedyOptimizeEdges();
        int edgesAfter = countEdges(graph);

        CHECK(compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)));
        CHECK(edgesAfter < edgesBefore);
        CHECK(graph.greedyOptimizeEdges().empty());
    }
}
