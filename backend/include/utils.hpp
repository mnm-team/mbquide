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

/**
 * @brief The plane/axis a graph vertex is measured in, as used throughout the
 * MBQC graph, flow-finding, and simplification code.
 *
 * `X`, `Y`, `Z` are single-axis Pauli measurements (always at angle 0 or π).
 * `XY`, `YZ`, `XZ` are arbitrary-angle measurements in the named plane.
 * `OUTPUT` marks a vertex as a pattern output (not measured; see
 * OutputAdjustmentMap instead). `UNDEFINED` is a sentinel for "no basis set".
 */
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

/// Default tolerance used by fAlmostEqual()/cAlmostEqual() and throughout the
/// codebase wherever two angles or amplitudes are compared for equality.
const float TOLERANCE = 0.0001f;

// Functions for radians

/// Whether two floats are within `tolerance` of each other.
bool fAlmostEqual(float a, float b, float tolerance = TOLERANCE);
/// Whether two complex numbers are within `tolerance` of each other (by modulus of their difference).
bool cAlmostEqual(const std::complex<double>& a, const std::complex<double>& b, float tolerance = TOLERANCE);
/// Reduces an angle to the range [0, 2π).
float normalize_radians(float radians);
/// Formats an angle (after normalize_radians()) as a human-readable fraction of π
/// (e.g. "π/4", "3π/2"), or a decimal string if it isn't a recognized multiple of π/8.
std::string radiansToString(float radians);
/// Parses an angle string in the format produced by radiansToString() (e.g. "π/4") back into radians.
double parseAngle(const std::string& angleStr);
/// Evaluates a small arithmetic expression string used in QASM gate parameters
/// (supports `pi`/`π`/`e` constants and a single `+`, `-`, `*`, or `/` operator).
double parseMathValue(const std::string& expr);

// Functions for bases

/// Maps a MeasurementBasis to the PyZX vertex-type integer used by MBQC_Graph::exportToPYZXJsonFile().
int basis_to_t(MeasurementBasis basis);
/// Converts a MeasurementBasis to its short string name (e.g. "XY", "OUTPUT").
std::string basisToString(MeasurementBasis basis);
/// Parses a MeasurementBasis from its string name (as produced by basisToString()); throws std::invalid_argument if unrecognized.
MeasurementBasis parseBasis(const std::string& str);

// Matrix functions:

/// Prints a vector's elements space-separated on one line.
template<typename T>
void printVector(const std::vector<T>& vec) {
    for (const auto& value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

/// Prints a matrix row by row, with a `- ` marker line indicating the row count.
template<typename T>
void printMatrix(const std::vector<std::vector<T>>& matrix) {
    for (const auto& row : matrix) { std::cout << "- "; }
    std::cout << "\n";

    for (const auto& row : matrix) {
        printVector(row);
    }
    std::cout << std::endl;
}


/**
 * @brief Converts a rectangular `std::vector<std::vector<T>>` into an `Eigen::MatrixXd`.
 * @throws std::invalid_argument if the input is empty or not rectangular.
 */
template<typename T>
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

/// Converts an `Eigen::MatrixXd` back into a `std::vector<std::vector<double>>`.
std::vector<std::vector<double>> eigenToDoubleMatrix(Eigen::MatrixXd& A);


/**
 * @brief Computes the matrix inverse of `matrix` via Eigen.
 * @return The inverse as a `std::vector<std::vector<double>>`, or an empty
 * vector if `matrix` is singular (not invertible).
 */
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

/**
 * @brief Computes the transitive closure of a directed graph given as an
 * explicit node list and edge list, using the Boost Graph Library.
 * @param nodes The graph's vertex ids.
 * @param[in,out] edges The graph's edges on input; replaced with the edges of
 * the transitive closure on output.
 * @return `false` if the graph contains a cycle (not a DAG), in which case
 * `edges` is left unmodified; `true` otherwise.
 */
bool computeTransitiveClosure(const std::vector<int>& nodes, std::vector<std::pair<int,int>>& edges);

// GF2 functions

/// A matrix over GF(2) (entries are 0 or 1, arithmetic is mod 2), stored as rows of ints.
/// Used throughout Flow.hpp/Flow.cpp for the linear-algebraic Pauli flow-finding algorithm.
using GF2Mat = std::vector<std::vector<int>>;
/// The n x n identity matrix over GF(2).
GF2Mat gf2Eye(int n);
/// An r x c all-zero matrix over GF(2).
GF2Mat gf2Zeros(int r, int c);
/// Matrix product `A * B` over GF(2).
GF2Mat gf2Mul(const GF2Mat& A, const GF2Mat& B);
/// Horizontal concatenation `[A | B]` (same row count required).
GF2Mat gf2Hcat(const GF2Mat& A, const GF2Mat& B);
/// Vertical concatenation `[A ; B]` (same column count required).
GF2Mat gf2Vcat(const GF2Mat& A, const GF2Mat& B);
/// Extracts columns `[colStart, colEnd)` of `A`.
GF2Mat gf2ColSlice(const GF2Mat& A, int colStart, int colEnd);
/// XORs `src` into row `dst` of `mat` in place (`mat[dst] ^= src`, element-wise).
void gf2XorRowInto(GF2Mat& mat, int dst, const std::vector<int>& src);
/**
 * @brief Reduces `mat` to reduced row echelon form (RREF) over GF(2) in place,
 * treating only columns `[0, nActiveCols)` as pivot columns.
 * @param[in,out] mat The matrix to reduce, modified in place.
 * @param nActiveCols Number of leading columns eligible to be pivots.
 * @param[out] pivotCols The pivot column index found for each pivot, in order.
 * @param[out] pivotRows The row index of each pivot, in order (parallel to `pivotCols`).
 * @return The rank of `mat` restricted to the active columns.
 */
int gf2RREF(GF2Mat& mat, int nActiveCols, std::vector<int>& pivotCols, std::vector<int>& pivotRows);
/**
 * @brief Computes a right inverse and a kernel basis of `M` over GF(2), i.e.
 * `C0` with `M * C0 = I` and `F` whose columns span `ker(M)`.
 * @param M The matrix to invert/analyze.
 * @param[out] C0 A right inverse of `M`.
 * @param[out] F A basis for the kernel of `M`, as matrix columns.
 * @return `false` if `M` is not right-invertible (rank < row count), in which
 * case `C0`/`F` are left unspecified.
 */
bool gf2RightInvAndKernel(const GF2Mat& M, GF2Mat& C0, GF2Mat& F);
/// Whether the directed graph with adjacency matrix `NC` (over GF(2), `NC[u][v]==1` meaning an edge u->v) is acyclic, via Kahn's algorithm.
bool gf2IsDAG(const GF2Mat& NC);

#endif
