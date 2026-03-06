#include "doctest.h"
#include "test_helpers.hpp"
#include "Quantum_Circuit.hpp"
#include "QASM_Parser.hpp"
#include "OutputAdjustments.hpp"
#include "ZX_Graph.hpp"
#include <cmath>
#include <cmath>



TEST_CASE("Single-Qubit Output Adjustments - Basic Gates") {

    SUBCASE("X gate") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("X");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nx q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("Z gate") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("Z");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nz q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("H gate") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("H");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nh q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("S gate") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("S");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\ns q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("-S gate") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("-S");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nrz(-1.57079632679) q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }
}



TEST_CASE("Single-Qubit Output Adjustments - RZ Rotations") {

    SUBCASE("RZ(pi)") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RZ", M_PI);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nrz(3.141592653589793) q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("RZ(pi/2) equals S") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RZ", M_PI/2);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\ns q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("RZ(0) equals identity") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RZ", 0.0);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("RZ(-pi/2) equals -S") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RZ", -M_PI/2);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nrz(-1.57079632679) q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

}


TEST_CASE("Single-Qubit Output Adjustments - RX Rotations") {

    SUBCASE("RX(pi)") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RX", M_PI);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nrx(3.141592653589793) q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("RX(0) equals identity") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RX", 0.0);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

}


TEST_CASE("Single-Qubit Output Adjustments - Composition") {

    SUBCASE("XX = I") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("X");
        oam.adjustOutput("X");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("ZZ = I") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("Z");
        oam.adjustOutput("Z");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("H H = I") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("H");
        oam.adjustOutput("H");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("HZH = X") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("H");
        oam.adjustOutput("Z");
        oam.adjustOutput("H");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nx q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }

    SUBCASE("RZ(pi) equals Z") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("RZ", M_PI);

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());
        QuantumCircuit ref =
            QASMParser("", "qreg q[1];\nz q[0];").parse();

        CHECK(compareTensors(zx, ZXGraph::fromQuantumCircuit(ref)));
    }
}

TEST_CASE("Repeated and Mixed Adjustments") {

    SUBCASE("XZX combination") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("X");
        oam.adjustOutput("Z");
        oam.adjustOutput("X");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());

        QuantumCircuit refCircuit =
            QASMParser("", 
                "qreg q[1];\n"
                "x q[0];\n"
                "z q[0];\n"
                "x q[0];").parse();

        ZXGraph ref = ZXGraph::fromQuantumCircuit(refCircuit);
        CHECK(compareTensors(zx, ref));
    }

    SUBCASE("ZZZ = Z") {
        OutputAdjustmentMap oam;
        oam.adjustOutput("Z");
        oam.adjustOutput("Z");
        oam.adjustOutput("Z");

        ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());

        QuantumCircuit refCircuit =
            QASMParser("", 
                "qreg q[1];\n"
                "z q[0];").parse();

        ZXGraph ref = ZXGraph::fromQuantumCircuit(refCircuit);
        CHECK(compareTensors(zx, ref));
    }
}


TEST_CASE("No Adjustment Case") {

    OutputAdjustmentMap oam;
    ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());

    QuantumCircuit refCircuit =
        QASMParser("", "qreg q[1];").parse();

    ZXGraph ref = ZXGraph::fromQuantumCircuit(refCircuit);
    CHECK(compareTensors(zx, ref));
}



TEST_CASE("Single-Qubit Output Adjustments - Large Clifford+T Stress Test") {

    OutputAdjustmentMap oam;

    for (int i = 0; i < 8; ++i) {
        oam.adjustOutput("H");
        oam.adjustOutput("X");
        oam.adjustOutput("Z");
        oam.adjustOutput("RZ", M_PI);
        oam.adjustOutput("RX", M_PI);
        oam.adjustOutput("RZ", M_PI/2);
        oam.adjustOutput("RZ", -M_PI/2);
        oam.adjustOutput("RX", M_PI/2);
        oam.adjustOutput("RX", -M_PI/2);
        oam.adjustOutput("RZ", M_PI/4);
        oam.adjustOutput("RZ", -M_PI/4);
        oam.adjustOutput("RX", M_PI/4);
        oam.adjustOutput("RX", -M_PI/4);
        oam.adjustOutput("S");
        oam.adjustOutput("-S");
        oam.adjustOutput("H");
        oam.adjustOutput("RZ", M_PI/2);
        oam.adjustOutput("H");
        oam.adjustOutput("RX", M_PI/2);
    }

    ZXGraph zx = ZXGraph::fromQuantumCircuit(oam.toCircuit());

    std::stringstream qasm;
    qasm << "qreg q[1];\n";

    for (int i = 0; i < 8; ++i) {

        qasm << "h q[0];\n";
        qasm << "x q[0];\n";
        qasm << "z q[0];\n";

        qasm << "rz(" << M_PI << ") q[0];\n";
        qasm << "rx(" << M_PI << ") q[0];\n";

        qasm << "rz(" << M_PI/2 << ") q[0];\n";
        qasm << "rz(" << -M_PI/2 << ") q[0];\n";
        qasm << "rx(" << M_PI/2 << ") q[0];\n";
        qasm << "rx(" << -M_PI/2 << ") q[0];\n";

        qasm << "rz(" << M_PI/4 << ") q[0];\n";
        qasm << "rz(" << -M_PI/4 << ") q[0];\n";
        qasm << "rx(" << M_PI/4 << ") q[0];\n";
        qasm << "rx(" << -M_PI/4 << ") q[0];\n";

        qasm << "s q[0];\n";
        qasm << "rz(" << -M_PI/2 << ") q[0];\n";  // -S

        qasm << "h q[0];\n";
        qasm << "rz(" << M_PI/2 << ") q[0];\n";
        qasm << "h q[0];\n";
        qasm << "rx(" << M_PI/2 << ") q[0];\n";
    }

    QuantumCircuit ref =
        QASMParser("", qasm.str()).parse();

    ZXGraph refZX = ZXGraph::fromQuantumCircuit(ref);

    CHECK(compareTensors(zx, refZX));
}