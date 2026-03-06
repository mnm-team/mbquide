#include "utils.hpp"

// ##### RADIANS FUNCTIONS: #######
bool fAlmostEqual(float a, float b, float tolerance) {
    return fabs(a - b) < tolerance;
}

bool cAlmostEqual(const std::complex<double>& a, const std::complex<double>& b, float tolerance) {
    return std::abs(a - b) < tolerance;
}

float normalize_radians(float radians) {
    // Normalize to [0, 2pi)
    radians = fmod(radians, 2*M_PI);
    if (radians < 0) {
        radians += 2*M_PI;
    }
    return radians;
}

std::string radiansToString(float radians) {

    float normalized = normalize_radians(radians);

    if (fAlmostEqual(normalized, M_PI / 4)) {
        return "\u03c0/4";
    } else if (fAlmostEqual(normalized, M_PI / 2)) {
        return "\u03c0/2";
    } else if (fAlmostEqual(normalized, 3 * M_PI / 4)) {
        return "3\u03c0/4";
    } else if (fAlmostEqual(normalized, M_PI)) {
        return "\u03c0";
    } else if (fAlmostEqual(normalized, 5 * M_PI / 4)) {
        return "5\u03c0/4";
    } else if (fAlmostEqual(normalized, 3 * M_PI / 2)) {
        return "3\u03c0/2";
    } else if (fAlmostEqual(normalized, 7 * M_PI / 4)) {
        return "7\u03c0/4";
    } else if (fAlmostEqual(normalized, 2 * M_PI)) {
        return "2\u03c0";
    } else if (fAlmostEqual(normalized, 0)) {
        return "";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(5) << normalized;
    return oss.str();
}

double parseAngle(const std::string& angleStr) {
    if (angleStr == "") return 0.0;
    if (angleStr == "\u03c0") return M_PI;
    if (angleStr == "2\u03c0") return 2 * M_PI;
    if (angleStr == "\u03c0/2") return M_PI / 2;
    if (angleStr == "3\u03c0/2") return 3 * M_PI / 2;
    if (angleStr == "\u03c0/4") return M_PI / 4;
    if (angleStr == "3\u03c0/4") return 3 * M_PI / 4;
    if (angleStr == "5\u03c0/4") return 5 * M_PI / 4;
    if (angleStr == "7\u03c0/4") return 7 * M_PI / 4;
    return std::stod(angleStr);  // fallback for numeric string
}

// Used by QASM parser for the parameters
double parseMathValue(const std::string& expr) {
    std::string s = expr;
    
    // Trim whitespace
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    
    // Replace mathematical constants
    size_t pos;
    while ((pos = s.find("pi")) != std::string::npos) {
        s.replace(pos, 2, std::to_string(M_PI));
    }
    while ((pos = s.find("π")) != std::string::npos) {
        s.replace(pos, 2, std::to_string(M_PI));
    }
    while ((pos = s.find('e')) != std::string::npos) {
        // Make sure it's standalone 'e', not part of exponential notation
        if ((pos == 0 || !isdigit(s[pos-1])) && 
            (pos == s.length()-1 || !isdigit(s[pos+1]))) {
            s.replace(pos, 1, std::to_string(M_E));
        } else {
            break;
        }
    }
    
    // Handle simple binary operations (single operator only)
    for (size_t i = 1; i < s.length(); ++i) {
        char op = s[i];
        if (op == '/' || op == '*' || op == '+' || op == '-') {
            try {
                double left = std::stod(s.substr(0, i));
                double right = std::stod(s.substr(i + 1));
                
                switch(op) {
                    case '/': return left / right;
                    case '*': return left * right;
                    case '+': return left + right;
                    case '-': return left - right;
                }
            } catch (...) {
                continue;  // Not a valid split point
            }
        }
    }
    
    // If no operator found, just parse as number
    return std::stod(s);
}



// ##### BASIS FUNCTIONS: #######

int basis_to_t(MeasurementBasis basis) {
    switch (basis) {
        case MeasurementBasis::OUTPUT: return 0;
        case MeasurementBasis::X: return 1;
        case MeasurementBasis::Y: return 2;
        case MeasurementBasis::Z: return 3;
        case MeasurementBasis::XY: return 4;
        case MeasurementBasis::YZ: return 5;
        case MeasurementBasis::XZ: return 6;
        default: return -1;
    }
}

std::string basisToString(MeasurementBasis basis) {
    switch (basis) {
        case MeasurementBasis::OUTPUT: return "OUTPUT";
        case MeasurementBasis::X: return "X";
        case MeasurementBasis::Y: return "Y";
        case MeasurementBasis::Z: return "Z";
        case MeasurementBasis::XY: return "XY";
        case MeasurementBasis::YZ: return "YZ";
        case MeasurementBasis::XZ: return "XZ";
        default: return "Unknown";
    }
}

MeasurementBasis parseBasis(const std::string& str) {
    if (str == "OUTPUT") return MeasurementBasis::OUTPUT;
    if (str == "X") return MeasurementBasis::X;
    if (str == "Y") return MeasurementBasis::Y;
    if (str == "Z") return MeasurementBasis::Z;
    if (str == "XY") return MeasurementBasis::XY;
    if (str == "XZ") return MeasurementBasis::XZ;
    if (str == "YZ") return MeasurementBasis::YZ;
    throw std::invalid_argument("Unknown basis: " + str);
}



// ##### FLOW FUNCTIONS: #######


std::vector<std::vector<double>> eigenToDoubleMatrix(Eigen::MatrixXd& A) {
    std::vector<std::vector<double>> result(A.rows(), std::vector<double>(A.cols()));
    for (int i = 0; i < A.rows(); ++i) {
        for (int j = 0; j < A.cols(); ++j) {
            result[i][j] = A(i, j);
        }
    }
    return result;
}


// Efficient transitive closure computation using Boost Graph Library
bool computeTransitiveClosure(
    const std::vector<int>& nodes,
    std::vector<std::pair<int,int>>& edges)
{
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    
    // Create vertex mapping
    std::unordered_map<int, Vertex> nodeToVertex;
    std::vector<int> vertexToNode;
    
    // Add all nodes as vertices
    Graph graph;
    for (int node : nodes) {
        Vertex v = boost::add_vertex(graph);
        nodeToVertex[node] = v;
        vertexToNode.push_back(node);
    }
    
    // Add edges to the graph
    for (const auto& edge : edges) {
        auto it1 = nodeToVertex.find(edge.first);
        auto it2 = nodeToVertex.find(edge.second);
        if (it1 != nodeToVertex.end() && it2 != nodeToVertex.end()) {
            boost::add_edge(it1->second, it2->second, graph);
        }
    }
    
    // Check for cycles using topological sort
    try {
        std::vector<Vertex> topoOrder;
        boost::topological_sort(graph, std::back_inserter(topoOrder));
    } catch (boost::not_a_dag&) {
        // Graph contains a cycle
        return false;
    }
    
    // Compute transitive closure using Boost
    Graph closureGraph;
    boost::transitive_closure(graph, closureGraph);
    
    // Extract edges from transitive closure
    edges.clear();
    auto edgeRange = boost::edges(closureGraph);
    for (auto it = edgeRange.first; it != edgeRange.second; ++it) {
        Vertex src = boost::source(*it, closureGraph);
        Vertex tgt = boost::target(*it, closureGraph);
        
        // Convert back to node IDs
        if (src < vertexToNode.size() && tgt < vertexToNode.size()) {
            edges.emplace_back(vertexToNode[src], vertexToNode[tgt]);
        }
    }
    
    return true;
}