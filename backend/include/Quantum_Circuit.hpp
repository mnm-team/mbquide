#ifndef QUANTUMCIRCUIT_HPP
#define QUANTUMCIRCUIT_HPP

#include <string>
#include <vector>
#include <iostream>

struct Gate {
    std::string name;
    std::vector<int> qubits;
    std::vector<int> clbits;
    std::vector<double> params;
};

class QuantumCircuit {
public:
    int num_qubits = 0;
    int num_clbits = 0;
    std::vector<Gate> gates;

    void addGate(const std::string& name, const std::vector<int>& qubits, const std::vector<int>& clbits, const std::vector<double>& params);
    void printCircuit() const;
};

#endif
