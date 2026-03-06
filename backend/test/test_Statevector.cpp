#include "doctest.h"
#include "Statevector.hpp"
#include "utils.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <complex>

using cplx = std::complex<double>;

TEST_CASE("Initialization") {
    SUBCASE("Single qubit initialization") {
        StatevectorSimulator sim(1);
        auto sv = sim.get_statevector();
        
        CHECK(sim.get_num_qubits() == 1);
        CHECK(sv.size() == 2);
        CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(0.0, 0.0)));
    }
    
    SUBCASE("Multi-qubit initialization") {
        StatevectorSimulator sim(3);
        auto sv = sim.get_statevector();
        
        CHECK(sim.get_num_qubits() == 3);
        CHECK(sv.size() == 8);
        CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
        
        for (int i = 1; i < 8; i++) {
            CHECK(cAlmostEqual(sv[i], std::complex<double>(0.0, 0.0)));
        }
    }
    
    SUBCASE("Invalid initialization") {
        CHECK_THROWS_AS(StatevectorSimulator(-1), std::invalid_argument);
    }
}

TEST_CASE("Some small operations, comparison with string") {
    StatevectorSimulator sim(2);
    
    CHECK(sim.getStatevectorBraKet() == "(1)|00>");

    sim.X(0);
    CHECK(sim.getStatevectorBraKet() == "(1)|01>");

    sim.H(1);
    CHECK(sim.getStatevectorBraKet() == "(0.707107)|01> + (0.707107)|11>");
    
}


TEST_CASE("X Gate (Pauli-X)") {
    SUBCASE("Single qubit X gate") {
        StatevectorSimulator sim(1);
        sim.X(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(0.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(1.0, 0.0)));
    }
    
    SUBCASE("X gate on multi-qubit system") {
        StatevectorSimulator sim(2);
        sim.X(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(0.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(1.0, 0.0)));
        CHECK(cAlmostEqual(sv[2], std::complex<double>(0.0, 0.0)));
        CHECK(cAlmostEqual(sv[3], std::complex<double>(0.0, 0.0)));
    }
    
    SUBCASE("Double X gate returns to original state") {
        StatevectorSimulator sim(1);
        sim.X(0);
        sim.X(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(0.0, 0.0)));
    }
}

TEST_CASE("Y Gate (Pauli-Y)") {
    SUBCASE("Single qubit Y gate") {
        StatevectorSimulator sim(1);
        sim.Y(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(0.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(0.0, 1.0)));
    }
    
    SUBCASE("Double Y gate returns to original state") {
        StatevectorSimulator sim(1);
        sim.Y(0);
        sim.Y(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(0.0, 0.0)));
    }
}

TEST_CASE("Z Gate (Pauli-Z)") {
    SUBCASE("Z gate on |0> state") {
        StatevectorSimulator sim(1);
        sim.Z(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(0.0, 0.0)));
    }
    
    SUBCASE("Z gate on |1> state") {
        StatevectorSimulator sim(1);
        sim.X(0);
        sim.Z(0);
        auto sv = sim.get_statevector();
        
        CHECK(cAlmostEqual(sv[0], std::complex<double>(0.0, 0.0)));
        CHECK(cAlmostEqual(sv[1], std::complex<double>(-1.0, 0.0)));
    }
}

TEST_CASE("Hadamard Gate") {

    SUBCASE("|0> to |+> state") {
        StatevectorSimulator sim(1);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|0> + (0.707107)|1>");
    }

    SUBCASE("|1> to |-> state") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(1.0)|1>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|0> + (-0.707107)|1>");
    }

    SUBCASE("H H = Identity on |0>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    SUBCASE("H H = Identity on |1>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(1.0)|1>");
        sim.setState(state);
        sim.H(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    SUBCASE("H H H H = Identity on |1>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(1)|1>");
        sim.setState(state);
        sim.H(0);
        sim.H(0);
        sim.H(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    SUBCASE("H on first qubit of 2-qubit system") {
        StatevectorSimulator sim(2);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|00> + (0.707107)|01>");
    }

    SUBCASE("H on second qubit of 2-qubit system") {
        StatevectorSimulator sim(2);
        sim.H(1);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|00> + (0.707107)|10>");
    }

    SUBCASE("H on all qubits of 2-qubit system produces uniform superposition") {
        StatevectorSimulator sim(2);
        sim.H(0);
        sim.H(1);
        CHECK(sim.getStatevectorBraKet() == "(0.5)|00> + (0.5)|01> + (0.5)|10> + (0.5)|11>");
    }

    SUBCASE("H on all qubits of 3-qubit system produces uniform superposition") {
        StatevectorSimulator sim(3);
        sim.H(0);
        sim.H(1);
        sim.H(2);
        CHECK(sim.getStatevectorBraKet() == 
            "(0.353553)|000> + (0.353553)|001> + (0.353553)|010> + (0.353553)|011> + "
            "(0.353553)|100> + (0.353553)|101> + (0.353553)|110> + (0.353553)|111>");
    }

    SUBCASE("H does not affect untargeted qubits - only qubit 0 of 3-qubit system") {
        StatevectorSimulator sim(3);
        auto state = StatevectorSimulator::parseBraKet("(1)|100>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|100> + (0.707107)|101>");
    }

    SUBCASE("H X H = Z: applying to |0> gives |0>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.X(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    SUBCASE("H X H = Z: applying to |1> gives -|1>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(1)|1>");
        sim.setState(state);
        sim.H(0);
        sim.X(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(-1)|1>");
    }

    SUBCASE("H Z H = X: applying to |0> gives |1>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.Z(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    SUBCASE("H Z H = X: applying to |1> gives |0>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(1.0)|1>");
        sim.setState(state);
        sim.H(0);
        sim.Z(0);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    SUBCASE("H on |-> state returns |1>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|0> + (-0.707107)|1>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    SUBCASE("H on |+> state returns |0>") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|0> + (0.707107)|1>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    SUBCASE("H on complex superposition with imaginary amplitudes") {
        StatevectorSimulator sim(1);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|0> + (0.707107i)|1>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.5 + 0.5i)|0> + (0.5 - 0.5i)|1>");
    }

    SUBCASE("H on entangled-like 2-qubit state") {
        StatevectorSimulator sim(2);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|00> + (0.707107)|11>");
        sim.setState(state);
        sim.H(0);
        CHECK(sim.getStatevectorBraKet() == "(0.5)|00> + (0.5)|01> + (0.5)|10> + (-0.5)|11>");
    }

    SUBCASE("H on second qubit of Bell-like state") {
        StatevectorSimulator sim(2);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|00> + (0.707107)|11>");
        sim.setState(state);
        sim.H(1);
        CHECK(sim.getStatevectorBraKet() == "(0.5)|00> + (0.5)|01> + (0.5)|10> + (-0.5)|11>");
    }

    SUBCASE("H on second qubit of larger state") {
        StatevectorSimulator sim(4);
        auto state = StatevectorSimulator::parseBraKet("(0.353553)|0000> + (0.353553)|0001> + (0.353553)|0010> + (0.353553)|0011> + (-0.353553)|0100> + (0.353553)|0101> + (0.353553)|0110> + (-0.353553)|0111>");
        sim.setState(state);
        sim.H(2);
        auto result = sim.getStatevectorBraKet();
        CHECK((result == "(0.5)|0001> + (0.5)|0010> + (0.5)|0100> + (0.5)|0111>" || result == "(0.499999)|0001> + (0.499999)|0010> + (0.499999)|0100> + (0.499999)|0111>"));
    }
}


TEST_CASE("Normalization") {
    SUBCASE("State remains normalized after gates") {
        StatevectorSimulator sim(2);
        sim.H(0);
        sim.X(1);
        sim.Z(0);
        
        auto sv = sim.get_statevector();
        double total_prob = 0.0;
        for (const auto& amplitude : sv) {
            total_prob += std::norm(amplitude);
        }
        
        CHECK(std::abs(total_prob - 1.0) < TOLERANCE);
    }
}

TEST_CASE("Gate range validation") {
    StatevectorSimulator sim(3);
    
    CHECK_THROWS_AS(sim.X(-1), std::out_of_range);
    CHECK_THROWS_AS(sim.X(3), std::out_of_range);
    CHECK_THROWS_AS(sim.H(5), std::out_of_range);
    CHECK_THROWS_AS(sim.Z(-2), std::out_of_range);
}

TEST_CASE("Reset functionality") {
    StatevectorSimulator sim(2);
    
    sim.H(0);
    sim.X(1);
    sim.reset();
    
    auto sv = sim.get_statevector();
    CHECK(cAlmostEqual(sv[0], std::complex<double>(1.0, 0.0)));
    
    for (int i = 1; i < 4; i++) {
        CHECK(cAlmostEqual(sv[i], std::complex<double>(0.0, 0.0)));
    }
}


TEST_CASE("Measurement basis vectors") {
    

    SUBCASE("Measuring |0> in the X-basis") {
        StatevectorSimulator sim(1);
        
        int result = sim.measure_qubit_in_basis(0, MeasurementBasis::X, 0);
       

        CHECK((result == 0 || result == 1));
        
        // Check correct normalization
        auto sv = sim.get_statevector();
        double total_prob = 0.0;
        for (const auto& amplitude : sv) {
            total_prob += std::norm(amplitude);
        }
        
        CHECK(std::abs(total_prob - 1.0) < TOLERANCE);

        // Check correct collapsed state
        if (result == 0) {
            CHECK(sim.getStatevectorBraKet() == "(1)|0>");
        } else {
            CHECK(sim.getStatevectorBraKet() == "(1)|1>");
        }
        
    }

    SUBCASE("Measuring |0> in the X-basis gives ~50/50 results") {  // THIS IS A STATISTICAL TEST!
        const int N = 1000;
        const double EXPECTED = 0.5;
        const double TOL = 0.05;   // 5% deviation allowed

        int count_zero = 0;

        for (int i = 0; i < N; i++) {
            StatevectorSimulator sim(1);

            int result = sim.measure_qubit_in_basis(0, MeasurementBasis::X, 0);

            if (result == 0) count_zero++;
        }

        double freq = double(count_zero) / N;

        CHECK(freq == doctest::Approx(EXPECTED).epsilon(TOL));
    }


    SUBCASE("Measuring |0> in the Y-basis") {
        StatevectorSimulator sim(1);

        int result = sim.measure_qubit_in_basis(0, MeasurementBasis::Y, 0);


        CHECK((result == 0 || result == 1));

        // Check correct normalization
        auto sv = sim.get_statevector();
        double total_prob = 0.0;
        for (const auto& amplitude : sv) {
            total_prob += std::norm(amplitude);
        }
        CHECK(std::abs(total_prob - 1.0) < TOLERANCE);

        // Check correct collapsed state
        if (result == 0) {
            CHECK(sim.getStatevectorBraKet() == "(1)|0>");
        } else {
            CHECK(sim.getStatevectorBraKet() == "(1)|1>");
        }
    }

    SUBCASE("Tracing out qubit after measurement") {
        StatevectorSimulator sim(3);
        sim.X(0);
        sim.H(1);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|001> + (0.707107)|011>");

        int result = sim.measure(0, true);  // Measurement with tracing out qubit 0
        CHECK(result == 1);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|00> + (0.707107)|01>");

        result = sim.measure(0, true);  // Measurement with tracing out qubit 0 again
        CHECK((result == 1 || result == 0));  // result should be 50/50
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");

        result = sim.measure(0, true);  // when having one qubit, tracing out is not possible
        CHECK(result == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");

    }

    SUBCASE("Tracing out qubit after X measurement") {
        StatevectorSimulator sim(3);
        sim.H(0);
        sim.H(1);
        sim.CZ(2,0);
        sim.CZ(2,1);

        CHECK(sim.getStatevectorBraKet() == "(0.5)|000> + (0.5)|001> + (0.5)|010> + (0.5)|011>");

        int result = sim.measure_qubit_in_basis(2, MeasurementBasis::X, 0);
        CHECK((result == 1 || result == 0));
        CHECK(sim.getStatevectorBraKet() == "(0.5)|00> + (0.5)|01> + (0.5)|10> + (0.5)|11>");

        result = sim.measure_qubit_in_basis(1, MeasurementBasis::XY, 0);
        CHECK(result == 0);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|0> + (0.707107)|1>");

        result = sim.measure_qubit_in_basis(0, MeasurementBasis::XY, 0);
        CHECK(result == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
}

// ============================================================
// Targeted tests for measure_in_basis_vectors directly
// We bypass measure_qubit_in_basis and pass vectors manually,
// so we can isolate the U matrix construction and collapse logic.
// ============================================================

TEST_CASE("measure_in_basis_vectors - direct vector tests") {

    // Helper: build complex vector
    auto c = [](double r, double i) { return std::complex<double>(r, i); };
    const double s = 1.0 / std::sqrt(2.0);

    // ---- Test 1: Standard Z basis, |0> -> outcome 0 --------
    // psi0 = |0> = {1, 0}, psi1 = |1> = {0, 1}
    // Measuring |0> must give outcome 0, collapse to |0>
    SUBCASE("Z-basis vectors: |0> -> outcome 0, state stays |0>") {
        StatevectorSimulator sim(1);
        cplx psi0[2] = {c(1,0), c(0,0)};
        cplx psi1[2] = {c(0,0), c(1,0)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- Test 2: Standard Z basis, |1> -> outcome 1 --------
    SUBCASE("Z-basis vectors: |1> -> outcome 1, state collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.X(0); // state is now |1>
        cplx psi0[2] = {c(1,0), c(0,0)};
        cplx psi1[2] = {c(0,0), c(1,0)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    // ---- Test 3: X basis, |+> -> outcome 0 -----------------
    // psi0 = |+> = {1/sqrt2, 1/sqrt2}, psi1 = |-> = {1/sqrt2, -1/sqrt2}
    // State is |+>, so outcome must be 0, collapse to |0>
    SUBCASE("X-basis vectors: |+> -> outcome 0, collapses to |0>") {
        StatevectorSimulator sim(1);
        sim.H(0); // |+>
        cplx psi0[2] = {c(s,0), c(s,0)};
        cplx psi1[2] = {c(s,0), c(-s,0)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- Test 4: X basis, |-> -> outcome 1 -----------------
    SUBCASE("X-basis vectors: |-> -> outcome 1, collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.X(0);
        sim.H(0); // |->
        cplx psi0[2] = {c(s,0), c(s,0)};
        cplx psi1[2] = {c(s,0), c(-s,0)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    // ---- Test 5: Y basis, |+i> -> outcome 0 ----------------
    // psi0 = |+i> = {1/sqrt2, i/sqrt2}
    // psi1 = |-i> = {1/sqrt2, -i/sqrt2}
    // State is |+i>, outcome must be 0, collapse to |0>
    SUBCASE("Y-basis vectors: |+i> -> outcome 0, collapses to |0>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.S(0); // |+i>
        cplx psi0[2] = {c(s,0), c(0,s)};
        cplx psi1[2] = {c(s,0), c(0,-s)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- Test 6: Y basis, |-i> -> outcome 1 ----------------
    SUBCASE("Y-basis vectors: |-i> -> outcome 1, collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.Sdg(0); // |-i>
        cplx psi0[2] = {c(s,0), c(0,s)};
        cplx psi1[2] = {c(s,0), c(0,-s)};
        int r = sim.measure_in_basis_vectors(0, psi0, psi1);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    // ---- Test 7: Superposition - verify U is unitary -------
    // If we pass identity basis {|0>,|1>} but state is |+>,
    // we should get 50/50. This checks U is not doing something
    // crazy to the amplitudes before measurement.
    SUBCASE("Identity basis on |+>: probabilities are 50/50") {
        int count = 0;
        for (int i = 0; i < 100; i++) {
            StatevectorSimulator sim(1);
            sim.H(0);
            cplx psi0[2] = {c(1,0), c(0,0)};
            cplx psi1[2] = {c(0,0), c(1,0)};
            count += sim.measure_in_basis_vectors(0, psi0, psi1);
        }
        CHECK(count > 40);
        CHECK(count < 60);
    }

    // ---- Test 8: Normalization always preserved -------------
    SUBCASE("Normalization preserved after measure_in_basis_vectors") {
        // Use a 2-qubit system and measure qubit 0 in X basis, qubit 1 remains
        StatevectorSimulator sim(2);
        sim.H(0);
        sim.H(1);
        cplx psi0[2] = {c(s,0), c(s,0)};
        cplx psi1[2] = {c(s,0), c(-s,0)};
        sim.measure_in_basis_vectors(0, psi0, psi1);
        auto sv = sim.get_statevector();
        double prob = 0.0;
        for (auto& a : sv) prob += std::norm(a);
        CHECK(std::abs(prob - 1.0) < TOLERANCE);
    }
}


// ============================================================
// Deterministic basis measurement tests
// Key idea: if |psi> is an eigenstate of the measurement basis,
// the outcome must be deterministic (0 or 1) and the post-measurement
// state must be normalized. No statistics needed.
// ============================================================
TEST_CASE("Basis measurement - deterministic eigenstate tests") {

    // ---- Z basis (standard) --------------------------------
    SUBCASE("Z-basis: |0> -> outcome 0, collapses to |0>") {
        StatevectorSimulator sim(1);
        // |0> is +1 eigenstate of Z
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::Z, 0);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    SUBCASE("Z-basis: |1> -> outcome 1, collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.X(0);
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::Z, 0);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(-1)|1>");
    }

    // ---- X basis -------------------------------------------
    // |+> = (|0>+|1>)/sqrt(2) is the +1 eigenstate of X
    // |-> = (|0>-|1>)/sqrt(2) is the -1 eigenstate of X
    SUBCASE("X-basis: |+> -> outcome 0, collapses to |0>") {
        StatevectorSimulator sim(1);
        sim.H(0); // creates |+>
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::X, 0);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    SUBCASE("X-basis: |-> -> outcome 1, collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.X(0);
        sim.H(0); // creates |-> = (|0>-|1>)/sqrt(2)
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::X, 0);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    // ---- Y basis -------------------------------------------
    // |+i> = (|0>+i|1>)/sqrt(2) is the +1 eigenstate of Y -> outcome 0
    // |-i> = (|0>-i|1>)/sqrt(2) is the -1 eigenstate of Y -> outcome 1
    SUBCASE("Y-basis: |+i> -> outcome 0, collapses to |0>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.S(0); // S gate: |0>->|0>, |1>->i|1>, so H then S gives (|0>+i|1>)/sqrt(2)
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::Y, 0);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    SUBCASE("Y-basis: |-i> -> outcome 1, collapses to |1>") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.Sdg(0); // S† gate: gives (|0>-i|1>)/sqrt(2)
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::Y, 0);
        CHECK(r == 1);
        CHECK(sim.getStatevectorBraKet() == "(1)|1>");
    }

    // ---- XZ plane (polar angle alpha from Z toward X) -------
    // psi0 for XZ at alpha: cos(a/2)|0> + sin(a/2)|1>
    // Test with alpha=pi/2 -> (|0>+|1>)/sqrt(2) = |+>, should give outcome 0
    SUBCASE("XZ-basis alpha=pi/2: |+> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.H(0);
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::XZ, M_PI/2);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    // alpha=pi -> psi0 = sin(pi/2)|1> = |1>, so |1> -> outcome 0
    SUBCASE("XZ-basis alpha=pi: |1> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.X(0);
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::XZ, M_PI);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- XY plane (azimuthal angle alpha) -------------------
    // psi0 for XY at alpha: (|0> + e^{i*alpha}|1>)/sqrt(2)
    // alpha=0 -> psi0 = |+> -> measuring |+> gives outcome 0
    SUBCASE("XY-basis alpha=0: |+> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.H(0);
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::XY, 0);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    // alpha=pi -> psi0 = (|0> - |1>)/sqrt(2) = |-> -> measuring |-> gives outcome 0
    SUBCASE("XY-basis alpha=pi: |-> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.X(0);
        sim.H(0); // |->
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::XY, M_PI);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    // alpha=pi/2 -> psi0 = (|0> + i|1>)/sqrt(2) = |+i> -> measuring |+i> gives outcome 0
    SUBCASE("XY-basis alpha=pi/2: |+i> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.S(0); // |+i>
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::XY, M_PI/2);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- YZ plane -------------------------------------------
    // psi0 for YZ at alpha: cos(a/2)|0> + i*sin(a/2)|1>
    // alpha=pi/2 -> (|0>+i|1>)/sqrt(2) = |+i> -> outcome 0
    SUBCASE("YZ-basis alpha=pi/2: |+i> -> outcome 0") {
        StatevectorSimulator sim(1);
        sim.H(0);
        sim.S(0); // |+i>
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::YZ, M_PI/2);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }
    // alpha=0 -> psi0 = |0> -> outcome 0
    SUBCASE("YZ-basis alpha=0: |0> -> outcome 0") {
        StatevectorSimulator sim(1);
        int r = sim.measure_qubit_in_basis(0, MeasurementBasis::YZ, 0);
        CHECK(r == 0);
        CHECK(sim.getStatevectorBraKet() == "(1)|0>");
    }

    // ---- Normalization check after every basis measurement ---
    SUBCASE("Normalization preserved after all basis measurements") {
        auto check_norm = [](StatevectorSimulator& sim, MeasurementBasis b, double alpha) {
            sim.measure_qubit_in_basis(0, b, alpha);
            auto sv = sim.get_statevector();
            double prob = 0.0;
            for (auto& a : sv) prob += std::norm(a);
            CHECK(std::abs(prob - 1.0) < TOLERANCE);
        };
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::X,  0); }
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::Y,  0); }
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::Z,  0); }
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::XY, M_PI/3); }
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::XZ, M_PI/4); }
        { StatevectorSimulator s(1); s.H(0); check_norm(s, MeasurementBasis::YZ, M_PI/5); }
    }

    // ---- Orthogonal eigenstate gives outcome 1 (anti-eigenstate) ---
    // Measuring |0> in X-basis: |0> = (|+>+|->)/sqrt(2), 50/50
    // But measuring |-> in X-basis must give outcome 1
    SUBCASE("X-basis: |0> measured in X is 50/50 but NOT deterministic") {
        // Just confirm it doesn't always give 0
        int count = 0;
        for (int i = 0; i < 20; i++) {
            StatevectorSimulator sim(1);
            count += sim.measure_qubit_in_basis(0, MeasurementBasis::X, 0);
        }
        // If always 0, something is wrong with randomness or U matrix
        CHECK(count > 0);
        CHECK(count < 20);
    }
}



TEST_CASE("Testing setState Method") {
    SUBCASE("Simple") {
        StatevectorSimulator sim(2);
        Eigen::VectorXcd bell_state(4);
        bell_state << std::complex<double>(1.0/std::sqrt(2.0), 0.0), // |00>
                    std::complex<double>(0.0, 0.0),  // |01>
                    std::complex<double>(0.0, 0.0),  // |10>
                    std::complex<double>(1.0/std::sqrt(2.0), 0.0);  // |11>
        sim.setState(bell_state);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|00> + (0.707107)|11>");
    }

    SUBCASE("With parsing") {
        StatevectorSimulator sim(2);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|10> + (0.707107i)|01>");
        sim.setState(state);
        CHECK(sim.getStatevectorBraKet() == "(0.707107i)|01> + (0.707107)|10>");
    }

    SUBCASE("Subystem Simple") {
        StatevectorSimulator sim(6);
        std::vector<int> qubits = {1, 3};
        Eigen::VectorXcd bell_state(4);
        bell_state << std::complex<double>(1.0/std::sqrt(2.0), 0.0), // |00>
                    std::complex<double>(0.0, 0.0),  // |01>
                    std::complex<double>(0.0, 0.0),  // |10>
                    std::complex<double>(1.0/std::sqrt(2.0), 0.0);  // |11>
        sim.setStateSubsystem(qubits, bell_state);
        CHECK(sim.getStatevectorBraKet() == "(0.707107)|000000> + (0.707107)|001010>");
    }

    SUBCASE("Subystem With parsing") {
        StatevectorSimulator sim(6);
        auto state = StatevectorSimulator::parseBraKet("(0.707107)|10> + (0.707107i)|01>");
        std::vector<int> qubits = {1, 3};
        sim.setStateSubsystem(qubits, state);
        CHECK(sim.getStatevectorBraKet() == "(0.707107i)|000010> + (0.707107)|001000>");
    }
}