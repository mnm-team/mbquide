#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <Eigen/Dense>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transitive_closure.hpp>

enum class MeasurementBasis {
    X,
    Y,
    Z,
    XY,
    YZ,
    XZ,
    OUTPUT,
    UNDEFINED
};

const float TOLERANCE = 0.0001f;

// Functions for radians
bool fAlmostEqual(float a, float b, float tolerance = TOLERANCE);
bool cAlmostEqual(const std::complex<double>& a, const std::complex<double>& b, float tolerance = TOLERANCE);
float normalize_radians(float radians);
std::string radiansToString(float radians);
double parseAngle(const std::string& angleStr);
double parseMathValue(const std::string& expr);

// Functions for bases
int basis_to_t(MeasurementBasis basis);
std::string basisToString(MeasurementBasis basis);
MeasurementBasis parseBasis(const std::string& str);

// Matrix functions:
template<typename T>
void printVector(const std::vector<T>& vec) {
    for (const auto& value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

template<typename T>
void printMatrix(const std::vector<std::vector<T>>& matrix) {
    for (const auto& row : matrix) { std::cout << "- "; }
    std::cout << "\n";

    for (const auto& row : matrix) {
        printVector(row);
    }
    std::cout << std::endl;
}


template<typename T>
// Convert std::vector -> Eigen::MatrixXd
Eigen::MatrixXd stdMatrixToEigen(const std::vector<std::vector<T>>& matrix) {

    if (matrix.empty() || matrix[0].empty()) {
        throw std::invalid_argument("std::vector -> Eigen::MatrixXd: Input matrix must be non-empty");
    }
    for (const auto& row : matrix) {
        if (row.size() != matrix[0].size()) {
            throw std::invalid_argument("std::vector -> Eigen::MatrixXd: Input matrix must be rectangular");
        }
    }

    int n = matrix.size();
    int m = matrix[0].size();

    Eigen::MatrixXd A(n, m);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            A(i, j) = matrix[i][j];
        }
    }
    return A;
}

// Convert Eigen::MatrixXd -> std::vector<std::vector<double>>
std::vector<std::vector<double>> eigenToDoubleMatrix(Eigen::MatrixXd& A);


template<typename T>
std::vector<std::vector<double>> getInverse(const std::vector<std::vector<T>>& matrix) {
    
    // Convert std::vector -> Eigen::MatrixXd
    Eigen::MatrixXd A = stdMatrixToEigen(matrix);

    // Check if invertible
    if (!A.fullPivLu().isInvertible()) {
        return {};
    }

    // Invert using Eigen
    Eigen::MatrixXd Ainv = A.inverse();

    // Convert back Eigen::MatrixXd -> std::vector<double>
    std::vector<std::vector<double>> result = eigenToDoubleMatrix(Ainv);

    return result;
}

bool computeTransitiveClosure(const std::vector<int>& nodes, std::vector<std::pair<int,int>>& edges);



#endif