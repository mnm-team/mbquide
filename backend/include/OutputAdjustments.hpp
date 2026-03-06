#ifndef OUTPUTADJUSTMENT_HPP
#define OUTPUTADJUSTMENT_HPP

#include <Eigen/Dense>
#include <complex>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Quantum_Circuit.hpp"

const std::complex<double> J(0,1);

class OutputAdjustmentMap {
private:
    std::map<std::string, Eigen::Matrix2cd> adjustments_;  // {"X": Matrix2cd, "Z": Matrix2cd}
    
    static Eigen::Matrix2cd string2OperationMatrix(const std::string& operationString, double angle);
    static Eigen::Matrix2cd conjugate(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P);
    static Eigen::Matrix2cd conjugate_other(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P);
    static std::pair<std::string, int> identifyPauli(const Eigen::Matrix2cd& P);
    static Eigen::Matrix2cd parsePauli(const std::pair<std::string, int> pauliPair);

public:
    // Constructor
    OutputAdjustmentMap();
    
    // Apply operation to adjust outputs
    void adjustOutput(const std::string& operationString, double angle = 0);
    
    // Export
    std::string toString() const;
    json toJson() const;
    static OutputAdjustmentMap fromJson(const json& j);

    // Extract Gate
    QuantumCircuit toCircuit();
    
    // Reset to initial state
    void reset();
    
    // Direct access to internal map 
    std::map<std::string, Eigen::Matrix2cd>& getMap() { return adjustments_; }
    const std::map<std::string, Eigen::Matrix2cd>& getMap() const { return adjustments_; }

    // Check if has no change (is standard)
    bool isStandard() const;
};

#endif // OUTPUTADJUSTMENT_HPP