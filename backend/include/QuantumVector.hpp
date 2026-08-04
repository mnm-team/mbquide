#ifndef QUANTUM_VECTOR_HPP
#define QUANTUM_VECTOR_HPP

#include <Eigen/Dense>
#include <complex>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "utils.hpp"

// Bit-index arithmetic, bra-ket string <-> amplitude-vector conversion, and
// vector-equality checks shared by StatevectorSimulator and
// TensorNetworkSimulator. Both backends store amplitudes as a plain
// Eigen::VectorXcd (one dense vector for Statevector, one per entangled
// component for TensorNetwork) and only reach for these once they already
// have such a vector in hand - everything about *how* that vector is built
// and updated stays backend-specific.

using QVecC = Eigen::VectorXcd;

inline int get_bit(int number, int position) {
    return (number >> position) & 1;
}

inline int set_bit(int number, int position, int value) {
    if (value) return number | (1 << position);
    return number & ~(1 << position);
}

inline int remove_bit(int idx, int pos) {
    int lower_mask = (1 << pos) - 1;
    int upper_mask = ~((1 << (pos + 1)) - 1);
    int lower = idx & lower_mask;
    int upper = (idx & upper_mask) >> 1;
    return lower | upper;
}

inline std::complex<double> parseComplex(const std::string& s) {
    double real = 0.0, imag = 0.0;
    std::string trimmed = s;
    size_t start = trimmed.find_first_not_of(" \t");
    size_t end = trimmed.find_last_not_of(" \t");
    if (start != std::string::npos) trimmed = trimmed.substr(start, end - start + 1);

    trimmed.erase(
        std::remove_if(trimmed.begin(), trimmed.end(),
                    [](unsigned char c){ return std::isspace(c); }),
        trimmed.end()
    );

    if (trimmed.empty() || trimmed == "0") return std::complex<double>(0.0, 0.0);

    size_t iPos = trimmed.find('i');
    if (iPos != std::string::npos) {
        size_t plusPos = trimmed.find(" + ");
        size_t minusPos = trimmed.find(" - ");
        if (plusPos != std::string::npos) {
            real = std::stod(trimmed.substr(0, plusPos));
            std::string imagStr = trimmed.substr(plusPos + 3, iPos - plusPos - 3);
            imag = std::stod(imagStr);
        } else if (minusPos != std::string::npos) {
            real = std::stod(trimmed.substr(0, minusPos));
            std::string imagStr = trimmed.substr(minusPos + 3, iPos - minusPos - 3);
            imag = -std::stod(imagStr);
        } else {
            std::string imagStr = trimmed.substr(0, iPos);
            imag = imagStr.empty() || imagStr == "+" ? 1.0 : imagStr == "-" ? -1.0 : std::stod(imagStr);
        }
    } else {
        real = std::stod(trimmed);
    }

    return std::complex<double>(real, imag);
}

// Parses strings like "(0.707107)|00> + (0.707107i)|11>" into a dense
// amplitude vector. Named distinctly from the per-class `parseBraKet`
// static wrappers so those can forward to this without recursing.
inline QVecC parseBraKetVector(const std::string& braket) {
    std::vector<std::pair<std::complex<double>, size_t>> terms;
    size_t num_qubits_local = 0;
    size_t pos = 0;
    while (pos < braket.length()) {
        while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos;
        if (pos < braket.length() && braket[pos] == '+') { ++pos; while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos; }
        if (pos >= braket.length() || braket[pos] != '(') break;
        size_t start = pos + 1;
        size_t end = braket.find(')', start);
        if (end == std::string::npos) break;
        std::string ampStr = braket.substr(start, end - start);
        std::complex<double> amplitude = parseComplex(ampStr);
        pos = end + 1;
        while (pos < braket.length() && std::isspace((unsigned char)braket[pos])) ++pos;
        if (pos >= braket.length() || braket[pos] != '|') break;
        ++pos;
        start = pos;
        end = braket.find('>', start);
        if (end == std::string::npos) break;
        std::string ketStr = braket.substr(start, end - start);
        num_qubits_local = std::max(num_qubits_local, ketStr.length());
        size_t index = 0;
        for (char c : ketStr) { index = (index << 1) | (c - '0'); }
        terms.push_back({amplitude, index});
        pos = end + 1;
    }

    if (terms.empty()) return QVecC();

    size_t dim = 1ULL << num_qubits_local;
    QVecC result = QVecC::Zero(dim);
    for (const auto& pr : terms) {
        if (pr.second < dim) result(static_cast<int>(pr.second)) = pr.first;
    }
    return result;
}

inline std::string vectorToBraKet(const QVecC& statevector) {
    auto fmt = [](std::complex<double> c) {
        std::ostringstream out;
        double r = c.real();
        double im = c.imag();
        if (std::abs(r) < TOLERANCE) r = 0;
        if (std::abs(im) < TOLERANCE) im = 0;
        if (r == 0 && im == 0) return std::string("0");
        out << "(";
        if (r != 0) out << r;
        if (im != 0) {
            if (im > 0 && r != 0) out << " + " << im << "i";
            else if (im < 0) out << " - " << std::abs(im) << "i";
            else if (r == 0) out << im << "i";
        }
        out << ")";
        return out.str();
    };

    size_t dim = statevector.size();
    size_t nq = 0;
    while ((1ULL << nq) < dim) nq++;
    std::ostringstream out;
    bool printedAnything = false;

    for (size_t i = 0; i < dim; ++i) {
        std::complex<double> amp = statevector(static_cast<int>(i));
        std::string ampStr = fmt(amp);
        if (ampStr == "0") continue;
        std::string ket;
        ket.reserve(nq);
        for (int q = (int)nq - 1; q >= 0; --q) ket.push_back(((i >> q) & 1) ? '1' : '0');
        if (!printedAnything) {
            out << ampStr << "|" << ket << ">";
            printedAnything = true;
        } else {
            out << " + " << ampStr << "|" << ket << ">";
        }
    }

    if (!printedAnything) out << "0";
    return out.str();
}

inline json vectorToJson(const QVecC& statevector) {
    json j = json::array();
    for (int i = 0; i < statevector.size(); ++i) {
        const std::complex<double>& c = statevector(i);
        j.push_back(json::array({c.real(), c.imag()}));
    }
    return j;
}

inline bool vectorsEqual(const QVecC& a, const QVecC& b, double tolerance = TOLERANCE) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i) {
        if (std::abs(a(i) - b(i)) > tolerance)
            return false;
    }
    return true;
}

inline bool vectorsEqualUpToGlobalPhase(const QVecC& a, const QVecC& b, double tolerance = TOLERANCE) {
    if (a.size() != b.size()) return false;

    std::complex<double> phase_factor(1.0, 0.0);
    bool phase_found = false;

    for (int i = 0; i < a.size(); ++i) {
        std::complex<double> amp_a = a(i);
        std::complex<double> amp_b = b(i);

        if (!phase_found) {
            if (std::abs(amp_a) > tolerance && std::abs(amp_b) > tolerance) {
                std::complex<double> ratio = amp_b / amp_a;
                phase_factor = ratio / std::abs(ratio);
                phase_found = true;
            } else if (std::abs(amp_a) > tolerance || std::abs(amp_b) > tolerance) {
                return false;
            }
            continue;
        }

        if (std::abs(amp_a * phase_factor - amp_b) > tolerance)
            return false;
    }

    return true;
}

#endif // QUANTUM_VECTOR_HPP
