#ifndef QASMPARSER_HPP
#define QASMPARSER_HPP

#include "Quantum_Circuit.hpp"
#include "utils.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <regex>

/**
 * @brief A small regex-based parser for a practical subset of OpenQASM 2.0,
 * producing a QuantumCircuit.
 *
 * Supports `qreg`/`creg` declarations (with automatic offset tracking across
 * multiple registers), `measure a[i] -> b[j];`, `//` comments, and gate calls
 * with 0-3 qubit operands and optional comma-separated parenthesized
 * parameters (e.g. `rz(pi/2) q[0];`, `ccx q[0],q[1],q[2];`). Gate names are
 * passed through as-is to QuantumCircuit::addGate(); unrecognized gates are
 * simply not matched by the parser's regexes rather than rejected explicitly.
 */
class QASMParser {
public:
    /**
     * @brief Constructs a parser reading from either a file or an in-memory
     * QASM string (exactly one of the two must be non-empty).
     * @param filename Path to a `.qasm` file to read, or `""` to use `qasmText` instead.
     * @param qasmText QASM source text, or `""` to read from `filename` instead.
     * @throws std::runtime_error if both or neither of `filename`/`qasmText` are given, or if `filename` can't be opened.
     */
    QASMParser(const std::string& filename = "", const std::string& qasmText = "");
    /// Parses the QASM source into a QuantumCircuit, stripping `//` comments and blank lines line by line.
    QuantumCircuit parse();

private:
    std::vector<std::string> qasm;
    QuantumCircuit circuit;


    std::map<std::string, int> qreg_offsets;  // Register name -> starting index
    std::map<std::string, int> creg_offsets;

    int current_qubit_offset = 0;
    int current_clbit_offset = 0;

    /// Maps a `(register name, local index)` pair to a global qubit index, using the offsets recorded from `qreg` declarations.
    int getQubitIndex(const std::string& reg_name, int local_index) {
        return qreg_offsets[reg_name] + local_index;
    }

    /// Maps a `(register name, local index)` pair to a global classical-bit index, using the offsets recorded from `creg` declarations.
    int getClbitIndex(const std::string& reg_name, int local_index) {
        return creg_offsets[reg_name] + local_index;
    }

    /// Parses one QASM statement (a register declaration, `measure`, or a gate call) and updates `circuit`/the register offset maps accordingly.
    void parseLine(const std::string& line);
    /// Extracts the bracketed index from a `name[index]` token, or -1 if the token doesn't match that pattern.
    int extractIndex(const std::string& token);
};

#endif
