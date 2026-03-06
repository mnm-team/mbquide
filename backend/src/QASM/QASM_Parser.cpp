#include "QASM_Parser.hpp"
#include <iostream>

QASMParser::QASMParser(const std::string& filename, const std::string& qasmText) {
    if (filename != "" && qasmText != "") {
        throw std::runtime_error("Please specify only one: filename OR qasmText");
    }
    if (filename != "") {
        std::ifstream file;
        file.open(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty())
                qasm.push_back(line);
        }
    }
    if (qasmText != "") {
        size_t start = 0, end;

        while ((end = qasmText.find('\n', start)) != std::string::npos) {
            qasm.emplace_back(qasmText.substr(start, end - start));
            start = end + 1;
        }
        qasm.emplace_back(qasmText.substr(start));
    }
}

QuantumCircuit QASMParser::parse() {
    for (std::string line : qasm) {
        line = line.substr(0, line.find("//"));
        if (!line.empty()) {
            parseLine(line);
        }
    }
    return circuit;
}

void QASMParser::parseLine(const std::string& line) {
    std::istringstream ss(line);
    std::string token;
    ss >> token;

    if (token == "OPENQASM" || token == "include") {
        return; // ignore
    }

    if (token == "qreg" || token == "creg") {
        std::string reg;
        ss >> reg;
        std::regex reg_pattern(R"(([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\];)");
        std::smatch match;
        if (std::regex_match(reg, match, reg_pattern)) {
            std::string reg_name = match[1];
            int size = std::stoi(match[2]);
            if (token == "qreg") {
                qreg_offsets[reg_name] = current_qubit_offset;
                current_qubit_offset += size;
                circuit.num_qubits += size;
            }
            if (token == "creg") {
                creg_offsets[reg_name] = current_clbit_offset;
                current_clbit_offset += size;
                circuit.num_clbits += size;
            }
        }
    } else if (token == "measure") {
        std::string from, arrow, to;
        ss >> from >> arrow >> to;

        // Extract register name and index from "regname[index]"
        std::regex idx_pattern(R"(([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\])");
        std::smatch match_from, match_to;

        if (std::regex_match(from, match_from, idx_pattern) &&
            std::regex_match(to, match_to, idx_pattern)) {
            std::string qreg_name = match_from[1];
            int q_local = std::stoi(match_from[2]);
            std::string creg_name = match_to[1];
            int c_local = std::stoi(match_to[2]);
            
            int q = getQubitIndex(qreg_name, q_local);
            int c = getClbitIndex(creg_name, c_local);
            circuit.addGate("measure", {q}, {c}, {});
        }
    } else {
        // Gates: h q[0]; or cx q[0],q[1];
        std::string rest;
        std::getline(ss, rest, ';');
        rest = token + rest;
        std::smatch match;

        // param gates
        std::regex param_gate_regex(R"((\w+)\(([^)]*)\)\s+([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\](?:,\s*([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\])?)");
        if (std::regex_search(rest, match, param_gate_regex)) {
            std::string name = match[1];
            std::string param_str = match[2];
            std::vector<double> params;
            std::istringstream param_stream(param_str);
            std::string val;
            while (std::getline(param_stream, val, ',')) {
                params.push_back(parseMathValue(val));
            }
            
            std::string reg1_name = match[3];
            int idx1 = std::stoi(match[4]);
            std::vector<int> qubits = {getQubitIndex(reg1_name, idx1)};
            
            if (match[5].matched) {
                std::string reg2_name = match[5];
                int idx2 = std::stoi(match[6]);
                qubits.push_back(getQubitIndex(reg2_name, idx2));
            }
            circuit.addGate(name, qubits, {}, params);
            return;
        }

        // non-parameterized
        std::regex gate_regex(R"((\w+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\](?:,\s*([a-zA-Z_][a-zA-Z0-9_]*)\[(\d+)\])?)");
        if (std::regex_search(rest, match, gate_regex)) {
            std::string name = match[1];
            std::string reg1_name = match[2];
            int idx1 = std::stoi(match[3]);
            std::vector<int> qubits = {getQubitIndex(reg1_name, idx1)};
            
            if (match[4].matched) {
                std::string reg2_name = match[4];
                int idx2 = std::stoi(match[5]);
                qubits.push_back(getQubitIndex(reg2_name, idx2));
            }
            circuit.addGate(name, qubits, {}, {});
        }
    }
}

int QASMParser::extractIndex(const std::string& token) {
    std::regex index_regex(R"([a-zA-Z_][a-zA-Z0-9_]*\[(\d+)\])");
    std::smatch match;
    if (std::regex_search(token, match, index_regex)) {
        return std::stoi(match[1]);
    }
    return -1;
}
