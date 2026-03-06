#ifndef QASMPARSER_HPP
#define QASMPARSER_HPP

#include "Quantum_Circuit.hpp"
#include "ZX_Graph.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <regex>

class QASMParser {
public:
    QASMParser(const std::string& filename = "", const std::string& qasmText = "");
    QuantumCircuit parse();

private:
    std::vector<std::string> qasm;
    QuantumCircuit circuit;


    std::map<std::string, int> qreg_offsets;  // Register name -> starting index
    std::map<std::string, int> creg_offsets;

    int current_qubit_offset = 0;
    int current_clbit_offset = 0;

    int getQubitIndex(const std::string& reg_name, int local_index) {
        return qreg_offsets[reg_name] + local_index;
    }
    
    int getClbitIndex(const std::string& reg_name, int local_index) {
        return creg_offsets[reg_name] + local_index;
    }

    void parseLine(const std::string& line);
    int extractIndex(const std::string& token);
};

#endif
