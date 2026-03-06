#include "Quantum_Circuit.hpp"

void QuantumCircuit::addGate(const std::string& name, const std::vector<int>& qubits, const std::vector<int>& clbits, const std::vector<double>& params) {
    gates.push_back({name, qubits, clbits, params});
}


void QuantumCircuit::printCircuit() const {
    std::cout << "Quantum Circuit with " << num_qubits << " qubits and " << num_clbits << " classical bits.\n";
    for (const auto& gate : gates) {
        std::cout << gate.name << " ";
        for (int q : gate.qubits)
            std::cout << "q[" << q << "] ";
        for (int c : gate.clbits)
            std::cout << "-> c[" << c << "] ";
        std::cout << "\n";
    }
}
