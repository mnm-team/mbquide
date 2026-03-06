#include "OutputAdjustments.hpp"



// ###### OPERATION DEFINITIONS ########


Eigen::Matrix2cd EigenMatrix_X = (Eigen::Matrix2cd() << 0,1,1,0).finished();
Eigen::Matrix2cd EigenMatrix_Y = (Eigen::Matrix2cd() << 0,-J,J,0).finished();
Eigen::Matrix2cd EigenMatrix_Z = (Eigen::Matrix2cd() << 1,0,0,-1).finished();
Eigen::Matrix2cd EigenMatrix_H = (1.0/sqrt(2.0)) * (Eigen::Matrix2cd() << 1, 1, 1, -1).finished();
Eigen::Matrix2cd EigenMatrix_I2 = Eigen::Matrix2cd::Identity();

Eigen::Matrix2cd Rx(double theta) {
    return cos(theta/2) * EigenMatrix_I2 - J * sin(theta/2) * EigenMatrix_X;
}

Eigen::Matrix2cd Rz(double theta) {
    return cos(theta/2) * EigenMatrix_I2 - J * sin(theta/2) * EigenMatrix_Z;
}



OutputAdjustmentMap::OutputAdjustmentMap() {
    reset();
}

void OutputAdjustmentMap::reset() {
    adjustments_["X"] = EigenMatrix_X;
    adjustments_["Z"] = EigenMatrix_Z;
}

bool OutputAdjustmentMap::isStandard() const {
    return (adjustments_.at("X") == EigenMatrix_X && adjustments_.at("Z") == EigenMatrix_Z);
}

// ###### STRING STUFF ########
Eigen::Matrix2cd OutputAdjustmentMap::string2OperationMatrix(const std::string& operationString, double angle) {
    std::string op = operationString;
    std::transform(op.begin(), op.end(), op.begin(), ::tolower);
    
    if (op == "z") {
        return EigenMatrix_Z;
    }
    else if (op == "x") {
        return EigenMatrix_X;
    }
    else if (op == "h") {
        return EigenMatrix_H;
    }
    else if (op == "rx") {
        return Rx(angle);
    }
    else if (op == "rz") {
        return Rz(angle);
    }
    else if (op == "s") {
        return Rz(M_PI/2);
    }
    else if (op == "-s") {
        return Rz(-M_PI/2);
    }
    else {
        throw std::invalid_argument("Unknown operation: " + operationString);
    }
}

std::pair<std::string, int> OutputAdjustmentMap::identifyPauli(const Eigen::Matrix2cd& P) {
    const double EPS = 1e-6;

    if ((P - EigenMatrix_X).norm() < EPS) return {"X", +1};
    if ((P + EigenMatrix_X).norm() < EPS) return {"X", -1};

    if ((P - EigenMatrix_Y).norm() < EPS) return {"Y", +1};
    if ((P + EigenMatrix_Y).norm() < EPS) return {"Y", -1};

    if ((P - EigenMatrix_Z).norm() < EPS) return {"Z", +1};
    if ((P + EigenMatrix_Z).norm() < EPS) return {"Z", -1};

    if ((P - EigenMatrix_I2).norm() < EPS) return {"I", +1};
    if ((P + EigenMatrix_I2).norm() < EPS) return {"I", -1};

    return {"Unknown", 0};
}

Eigen::Matrix2cd OutputAdjustmentMap::parsePauli(const std::pair<std::string, int> pauliPair) {

    std::string label = pauliPair.first;
    int sign = pauliPair.second;

    Eigen::Matrix2cd P;

    if (label == "X") P = EigenMatrix_X;
    else if (label == "Y") P = EigenMatrix_Y;
    else if (label == "Z") P = EigenMatrix_Z;
    else if (label == "I") P = EigenMatrix_I2;
    else {
        throw std::invalid_argument("Unknown Pauli label: " + label);
    }

    // Ensure sign is either +1 or -1
    if (sign != 1 && sign != -1)
        throw std::invalid_argument("Sign must be +1 or -1");

    return sign * P;
}



// ###### EXPORT STUFF (string, json) ########
std::string OutputAdjustmentMap::toString() const {
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, matrix] : adjustments_) {
        auto [pauli, sign] = identifyPauli(matrix);
        if (!first) oss << ", ";
        first = false;
        oss << key << ": " << (sign > 0 ? "+" : "-") << pauli;
    }
    return oss.str();
}

json OutputAdjustmentMap::toJson() const {
    json pauliMap;

    pauliMap["X"] = identifyPauli(adjustments_.at("X"));
    pauliMap["Z"] = identifyPauli(adjustments_.at("Z"));

    return pauliMap;
}

OutputAdjustmentMap OutputAdjustmentMap::fromJson(const json& j) {
    OutputAdjustmentMap oam = OutputAdjustmentMap();

    oam.adjustments_["X"] = parsePauli(j["X"]);
    oam.adjustments_["Z"] = parsePauli(j["Z"]);

    return oam;
}




// ###### MAIN ########

// Works analogous to stim: https://arxiv.org/pdf/2103.02202 (section 2.3)

// Conjugation: U * P * U†
Eigen::Matrix2cd OutputAdjustmentMap::conjugate(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P) {
    return U * P * U.adjoint();
}

// Conjugation: U† * P * U
Eigen::Matrix2cd OutputAdjustmentMap::conjugate_other(const Eigen::Matrix2cd& U, const Eigen::Matrix2cd& P) {
    return U.adjoint() * P * U;
}

// Main adjustment function
void OutputAdjustmentMap::adjustOutput(const std::string& operationString, double angle) {
    Eigen::Matrix2cd operation = string2OperationMatrix(operationString, angle);
    
    auto newX = conjugate(operation, adjustments_["X"]);
    auto newZ = conjugate(operation, adjustments_["Z"]);
    
    adjustments_["X"] = newX;
    adjustments_["Z"] = newZ;
}


// https://github.com/quantumlib/Stim/blob/197d9a64ea0734cf83a610601223f34fc6065ada/src/stim/util_top/circuit_vs_tableau.inl#L52
QuantumCircuit OutputAdjustmentMap::toCircuit() {
    QuantumCircuit circuit;
    circuit.num_qubits = 1;
    
    Eigen::Matrix2cd X_remaining = adjustments_["X"];
    auto [xp, xs] = identifyPauli(X_remaining);
    Eigen::Matrix2cd Z_remaining = adjustments_["Z"];
    auto [zp, zs] = identifyPauli(Z_remaining);

    // Add a gate and update the tableau
    auto apply = [&](const std::string& gate) {
        circuit.addGate(gate, {0}, {}, {});
        
        // Update the adjustments by applying the inverse gate
        Eigen::Matrix2cd U = string2OperationMatrix(gate, 0);
        X_remaining = conjugate_other(U, X_remaining); 
        Z_remaining = conjugate_other(U, Z_remaining);
        
        std::tie(xp, xs) = identifyPauli(X_remaining);
        std::tie(zp, zs) = identifyPauli(Z_remaining);
    };

    // Transform to XZ
    if (zp == "Y") {
        apply("S");
    }
    if (zp != "Z") {
        apply("H");
    }
    if (xp != "X") {
        apply("S");
    }


    // Fix pauli signs.
    if (zs == -1) {
        apply("H");
        apply("Z");
        apply("H");
    }
    if (xs == -1) {
        apply("Z");
    }
    
    // circuit.printCircuit();
    return circuit;
}

