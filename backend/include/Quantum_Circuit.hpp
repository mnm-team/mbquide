#ifndef QUANTUM_CIRCUIT_HPP
#define QUANTUM_CIRCUIT_HPP

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Gate {
    std::string        name;
    std::vector<int>   qubits;
    std::vector<int>   clbits;
    std::vector<double> params;
};


class QuantumCircuit {
public:
    int              num_qubits = 0;
    int              num_clbits = 0;
    std::vector<Gate> gates;

    QuantumCircuit() = default;

    QuantumCircuit(int nq, int nc = 0) : num_qubits(nq), num_clbits(nc) {}

    void addGate(const std::string&      name,
                 const std::vector<int>& qubits,
                 const std::vector<int>& clbits  = {},
                 const std::vector<double>& params = {})
    {
        gates.push_back({name, qubits, clbits, params});
    }

    void h  (int q)               { addGate("H",   {q}); }
    void x  (int q)               { addGate("X",   {q}); }
    void y  (int q)               { addGate("Y",   {q}); }
    void z  (int q)               { addGate("Z",   {q}); }
    void s  (int q)               { addGate("S",   {q}); }
    void sdg(int q)               { addGate("Sdg", {q}); }
    void t  (int q)               { addGate("T",   {q}); }
    void tdg(int q)               { addGate("Tdg", {q}); }
    void rx (int q, double theta) { addGate("Rx",  {q}, {}, {theta}); }
    void ry (int q, double theta) { addGate("Ry",  {q}, {}, {theta}); }
    void rz (int q, double theta) { addGate("Rz",  {q}, {}, {theta}); }
    void u3 (int q, double theta, double phi, double lam)
                                  { addGate("U3",  {q}, {}, {theta, phi, lam}); }

    void cx  (int c, int t)       { addGate("CX",   {c, t}); }
    void cnot(int c, int t)       { addGate("CNOT", {c, t}); }
    void cz  (int c, int t)       { addGate("CZ",   {c, t}); }
    void swap(int a, int b)       { addGate("SWAP", {a, b}); }

    void ccx(int c0, int c1, int t) { addGate("CCX", {c0, c1, t}); }
    void ccz(int c0, int c1, int t) { addGate("CCZ", {c0, c1, t}); }
    
    void measure(int q, int c)    { addGate("Measure", {q}, {c}); }

    void printCircuit() const {
        std::cout << "Quantum Circuit: " << num_qubits << " qubits, "
                  << num_clbits << " classical bits, "
                  << gates.size() << " gates.\n";
        for (const auto& g : gates) {
            std::cout << "  " << g.name;
            for (double p : g.params)
                std::cout << "(" << p << ")";
            for (int q : g.qubits)
                std::cout << " q[" << q << "]";
            for (int c : g.clbits)
                std::cout << " -> c[" << c << "]";
            std::cout << "\n";
        }
    }

    // Transpile to { J(α), CZ }
    //
    //  Returns a new QuantumCircuit in which every gate has been replaced by
    //  an equivalent sequence of J(α) and CZ gates.
    //  Throws std::invalid_argument for unrecognised gate names.
    //  Reference: Zilk et al., "A Compiler for Universal Photonic Quantum Computers", IEEE QCE 2022.  arXiv:2210.09251
    QuantumCircuit transpile() const {
        QuantumCircuit out(num_qubits, num_clbits);
        for (const auto& g : gates)
            transpileGate(out, g);
        return out;
    }

private:

    static constexpr double kPi  = M_PI;
    static constexpr double kPi2 = M_PI / 2.0;
    static constexpr double kPi4 = M_PI / 4.0;

    static void emitJ(QuantumCircuit& out, int q, double alpha) {
        out.addGate("J", {q}, {}, {alpha});
    }

    static void emitCZ(QuantumCircuit& out, int q0, int q1) {
        out.addGate("CZ", {q0, q1});
    }

    // H = J(0)
    static void dH(QuantumCircuit& out, int q) {
        emitJ(out, q, 0.0);
    }

    // X = J(0)·J(π)   [J(0)=H, J(π)=HZ → HZ·H = X]
    static void dX(QuantumCircuit& out, int q) {
        emitJ(out, q, 0.0);
        emitJ(out, q, kPi);
    }

    // Z = J(π)·J(0)   [HZ·H·H = HZ = Rz(π) = Z, up to phase]
    static void dZ(QuantumCircuit& out, int q) {
        emitJ(out, q, kPi);
        emitJ(out, q, 0.0);
    }

    // Y = iXZ  →  dZ then dX (global phase ignored)
    static void dY(QuantumCircuit& out, int q) {
        dZ(out, q);
        dX(out, q);
    }

    // Rz(θ) = J(0)·J(θ)   [H·(H·Rz(θ)) = Rz(θ)]
    static void dRz(QuantumCircuit& out, int q, double theta) {
        emitJ(out, q, theta);
        emitJ(out, q, 0.0);
    }

    // S  = Rz(π/2)
    static void dS(QuantumCircuit& out, int q)   { dRz(out, q,  kPi2); }
    // S† = Rz(-π/2)
    static void dSdg(QuantumCircuit& out, int q) { dRz(out, q, -kPi2); }
    // T  = Rz(π/4)
    static void dT(QuantumCircuit& out, int q)   { dRz(out, q,  kPi4); }
    // T† = Rz(-π/4)
    static void dTdg(QuantumCircuit& out, int q) { dRz(out, q, -kPi4); }

    // Rx(θ) = H·Rz(θ)·H = J(θ)·J(0)   [H cancels with leading H of J(0)]
    static void dRx(QuantumCircuit& out, int q, double theta) {
        emitJ(out, q, 0.0);
        emitJ(out, q, theta);
    }

    // Ry(θ) = Rz(-π/2)·Rx(θ)·Rz(π/2)
    //       = J(0)·J(-π/2) · J(θ)·J(0) · J(0)·J(π/2)
    //  After cancelling J(0)·J(0)=I at the two boundaries:
    //       = J(0)·J(π/2)·J(θ)·J(-π/2)   [4 J gates]
    static void dRy(QuantumCircuit& out, int q, double theta) {
        emitJ(out, q, -kPi2);
        emitJ(out, q, theta);
        emitJ(out, q,  kPi2);
        emitJ(out, q,  0.0);
    }

    // U3(θ,φ,λ) = Rz(φ)·Ry(θ)·Rz(λ)   — expand each factor
    static void dU3(QuantumCircuit& out, int q,
                    double theta, double phi, double lam) {
        dRz(out, q, lam);
        dRy(out, q, theta);
        dRz(out, q, phi);
    }


    // CNOT(c,t) = [I⊗H]·CZ·[I⊗H]
    static void dCNOT(QuantumCircuit& out, int c, int t) {
        emitJ(out, t, 0.0);
        emitCZ(out, c, t);
        emitJ(out, t, 0.0);
    }

    // SWAP(a,b) = CNOT(a,b)·CNOT(b,a)·CNOT(a,b)
    static void dSWAP(QuantumCircuit& out, int a, int b) {
        dCNOT(out, a, b);
        dCNOT(out, b, a);
        dCNOT(out, a, b);
    }


    // Toffoli / CCX — standard 6-CNOT decomposition (Nielsen & Chuang Fig 4.9)
    static void dCCX(QuantumCircuit& out, int c0, int c1, int t) {
        dH  (out, t);
        dCNOT(out, c1, t);  dTdg(out, t);
        dCNOT(out, c0, t);  dT  (out, t);
        dCNOT(out, c1, t);  dTdg(out, t);
        dCNOT(out, c0, t);
        dT(out, c1);        dT  (out, t);
        dH(out, t);
        dCNOT(out, c0, c1);
        dT  (out, c0);      dTdg(out, c1);
        dCNOT(out, c0, c1);
    }

    // CCZ(a,b,t) = H(t) · CCX(a,b,t) · H(t)
    static void dCCZ(QuantumCircuit& out, int c0, int c1, int t) {
        dH(out, t);
        dCCX(out, c0, c1, t);
        dH(out, t);
    }


    static void transpileGate(QuantumCircuit& out, const Gate& g) {
        // Case-fold name for matching
        std::string n = g.name;
        for (auto& c : n) c = static_cast<char>(std::toupper(c));

        if      (n == "H")                        dH  (out, g.qubits.at(0));
        else if (n == "X")                        dX  (out, g.qubits.at(0));
        else if (n == "Y")                        dY  (out, g.qubits.at(0));
        else if (n == "Z")                        dZ  (out, g.qubits.at(0));
        else if (n == "S")                        dS  (out, g.qubits.at(0));
        else if (n == "SDG"  || n == "S†")        dSdg(out, g.qubits.at(0));
        else if (n == "T")                        dT  (out, g.qubits.at(0));
        else if (n == "TDG"  || n == "T†")        dTdg(out, g.qubits.at(0));
        else if (n == "RX")   dRx(out, g.qubits.at(0), g.params.at(0));
        else if (n == "RY")   dRy(out, g.qubits.at(0), g.params.at(0));
        else if (n == "RZ")   dRz(out, g.qubits.at(0), g.params.at(0));
        else if (n == "U3" || n == "U")
            dU3(out, g.qubits.at(0), g.params.at(0), g.params.at(1), g.params.at(2));
        else if (n == "J")    // already in target basis
            emitJ(out, g.qubits.at(0), g.params.at(0));
        else if (n == "CX"   || n == "CNOT") dCNOT(out, g.qubits.at(0), g.qubits.at(1));
        else if (n == "CZ")                  emitCZ(out, g.qubits.at(0), g.qubits.at(1));
        else if (n == "SWAP")                dSWAP(out, g.qubits.at(0), g.qubits.at(1));
        else if (n == "CCX"  || n == "TOFFOLI")
            dCCX(out, g.qubits.at(0), g.qubits.at(1), g.qubits.at(2));
        else if (n == "CCZ")
            dCCZ(out, g.qubits.at(0), g.qubits.at(1), g.qubits.at(2));
        else if (n == "MEASURE")
            out.addGate(g.name, g.qubits, g.clbits, g.params); // pass-through
        else
            throw std::invalid_argument("QuantumCircuit::transpile: unknown gate '" + g.name + "'");
    }
};

#endif // QUANTUM_CIRCUIT_HPP