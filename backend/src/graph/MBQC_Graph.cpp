#include "utils.hpp"
#include "MBQC_Graph.hpp"
#include <limits>


MBQC_Graph::MBQC_Graph(int numNodes, const std::vector<int>& inputVertices, const std::vector<int>& outputVertices) : size(numNodes), inputs(inputVertices), outputs(outputVertices) {
    adjacencyMatrix.resize(size, std::vector<int>(size, 0));
    for (int u: outputs) {
        measurements[u] = std::make_pair(MeasurementBasis::OUTPUT, 0);
        outputAdjustments[u] = OutputAdjustmentMap();
    }
}

void MBQC_Graph::addEdge(int u, int v) {
    if (u >= 0 && v >= 0 && u < size && v < size) {
        adjacencyMatrix[u][v] = 1;
        adjacencyMatrix[v][u] = 1;
    }
}

void MBQC_Graph::setMeasurement(int node, MeasurementBasis basis, double angle) {

    angle = normalize_radians(angle);

    // Check if node in range
    if (node < 0 || node >= size) {
        std::cerr << "Issue while setting node " << node << ": node not in range of nodes in this graph!\n";
        return;   
    }

    //  Check if X,Y,Z have phase in {0, M_PI}
    if (basis == MeasurementBasis::X || basis == MeasurementBasis::Y || basis == MeasurementBasis::Z) {
        if (!(fAlmostEqual(angle, 0) || fAlmostEqual(angle, M_PI))) {
            std::cerr << "Issue while setting node " << node << ": basis is " << basisToString(basis) << " and angle is " << radiansToString(angle) << " which is not in {0, π}!\n";
            return;
        }
    }

    //  Check if node in not an output
    if (std::find(outputs.begin(), outputs.end(), node) != outputs.end()) {
        if (basis != MeasurementBasis::OUTPUT) {
            std::cerr << "Issue while setting node " << node << ": trying to set basis" << basisToString(basis) << " on an node that was declared an output!\n";
        }
        return;
    }

    measurements[node] = std::make_pair(basis, angle);

}


void MBQC_Graph::setOutputAdjustment(int node, OutputAdjustmentMap oam) {

    if (!isOutput(node)) {
        std::cerr << "Issue while setting OutputAdjustment on node " << node << ": This node is not an output!\n";
        return;
    }
    
    outputAdjustments[node] = oam;

}

const std::vector<std::vector<int>>& MBQC_Graph::getAdjacencyMatrix() const {
    return adjacencyMatrix;
}


std::vector<std::pair<int, int>> MBQC_Graph::getAllEdges() const {
        std::vector<std::pair<int, int>> edges;

        for (size_t i = 0; i < adjacencyMatrix.size(); ++i) {
            for (size_t j = 0; j < adjacencyMatrix[i].size(); ++j) {
                if (adjacencyMatrix[i][j] != 0) {
                    edges.push_back({i, j});
                }
            }
        }

        return edges;
}

std::vector<int> MBQC_Graph::getNeighbors(int u) const {
        std::vector<int> neighbors;
        if (u < 0 || u >= adjacencyMatrix.size()) {
            std::cerr << "Issue while getting neigbors of node " << u << ": Not in range of adjacencyMatrix!\n";
        }
        for (size_t i = 0; i < adjacencyMatrix[u].size(); ++i) {
            if (adjacencyMatrix[u][i] != 0) {
                neighbors.push_back(i);
            }
        }
        return neighbors;
}

std::unordered_set<int> MBQC_Graph::oddNeighborhood(const std::unordered_set<int>& S) const {
    std::unordered_set<int> odd;
    std::vector<int> parity(size, 0);

    for (int u : S) {
        for (int v = 0; v < size; ++v) {
            if (adjacencyMatrix[u][v] & 1) parity[v] ^= 1;
        }
    }

    for (int v = 0; v < size; ++v) if (parity[v]) odd.insert(v);
    
    return odd;
}

const int MBQC_Graph::getSize() const {
    return size;
}

const std::map<int, OutputAdjustmentMap>& MBQC_Graph::getOutputAdjustments() const {
    return outputAdjustments;
}

OutputAdjustmentMap& MBQC_Graph::getOutputAdjustment(int u) {
    return outputAdjustments.at(u);
}

OutputAdjustmentMap MBQC_Graph::getOutputAdjustment(int u) const {
    return outputAdjustments.at(u);
}

bool MBQC_Graph::isInput(int u) const {
    return std::find(inputs.begin(), inputs.end(), u) != inputs.end();
}

bool MBQC_Graph::isOutput(int u) const {
    bool inList = std::find(outputs.begin(), outputs.end(), u) != outputs.end();
    bool typeMatches = getMeasurement(u).first == MeasurementBasis::OUTPUT;
    if (typeMatches != inList) {
        if (typeMatches) {
            std::cerr << "Measurement type OUTPUT for index " << u << " is not in the Output List.\n";
        } else {
            std::cerr << "Measurement type " << basisToString(getMeasurement(u).first) << " does not match with index " << u << " being in the Output List.\n";
        }
    }
    return typeMatches;
}

std::vector<int> MBQC_Graph::getOutputs() const {
    return outputs;
}

std::vector<int> MBQC_Graph::getInputs() const {
    return inputs;
}

std::vector<int> MBQC_Graph::getNonOutputs() const {
    std::vector<int> nonOutputs;
    for (int v = 0; v < size; v++) {
        if (!isOutput(v)) nonOutputs.push_back(v);
    }
    return nonOutputs;
}

std::vector<int> MBQC_Graph::getNonInputs() const {
    std::vector<int> nonInputs;
    for (int v = 0; v < size; v++) {
        if (!isInput(v)) nonInputs.push_back(v);
    }
    return nonInputs;
}

// Give all measuremnt vertices (now all non-outputs, because inputs have a measurement basis)
std::vector<int> MBQC_Graph::mvertices() const {
    return getNonOutputs();
}

std::pair<MeasurementBasis, double> MBQC_Graph::getMeasurement(int node) const {
    return measurements.find(node)->second;
}


void MBQC_Graph::printGraph() const {
    std::cout << "\n-------------------------\n";
    std::cout << "Printing MBQC Graph:\n";

    std::cout << "Graph of size " << size << "\n";

    std::cout << "Inputs: ";
    for (int in : inputs) {
        std::cout << in << " ";
    }
    std::cout << "\nOutputs: ";
    for (int out : outputs) {
        std::cout << out << " ";
    }
    std::cout << "\n";

    std::cout << "Adjacency Matrix:\n";
    for (const auto& row : adjacencyMatrix) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\nMeasurement Data:\n";
    for (const auto& [node, data] : measurements) {
        std::cout << "Node " << node << ": Basis = " << basisToString(data.first) << ", Angle = " << data.second << "\n";
    }

    std::cout << "\nOutput Adjustment Data:";
    for (const auto& [outID, oa] : outputAdjustments) {
        std::cout << "\nOutput "<< outID << ":\t" << oa.toString() << "\n";
    }

    std::cout << "-------------------------\n";
}


// Encodes the full graph state into a compact string.
// Captures: size, adjacency, measurement bases+angles, and output adjustments.
std::string MBQC_Graph::stateHash() const {
    std::ostringstream oss;
    oss << size << "|";
    for (int i = 0; i < size; ++i)
        for (int j = i + 1; j < size; ++j)
            if (adjacencyMatrix[i][j])
                oss << i << "-" << j << ",";
    oss << "|";
    for (const auto& [node, data] : measurements)
        oss << node << ":" << static_cast<int>(data.first)
            << ":" << std::fixed << std::setprecision(6) << data.second << ";";
    oss << "|";
    for (const auto& [node, oa] : outputAdjustments)
        oss << node << ":" << oa.toString() << ";";
    return oss.str();
}


MBQC_Graph MBQC_Graph::clone() const {
    MBQC_Graph copy;

    copy.size = this->size;
    copy.adjacencyMatrix = this->adjacencyMatrix;
    copy.measurements = this->measurements;
    copy.inputs = this->inputs;
    copy.outputs = this->outputs;
    copy.outputAdjustments = this->outputAdjustments;

    return copy;
}



// ########## OPERATIONS ##############
void MBQC_Graph::localComplementation(int u) {

    auto [basis_u, angle_u] = getMeasurement(u);
    
    if (isInput(u)) {
        std::cerr << "Node " << u << " is an INPUT and thus local Complementation is not working!\n";
        return;
    }

    // Step 1: Identify neighbors of u
    std::vector<int> neighbors;
    for (int v = 0; v < size; ++v) {
        if (adjacencyMatrix[u][v]) {
            neighbors.push_back(v);
        }
    }

    // Step 2: Toggle edges between neighbors
    for (size_t i = 0; i < neighbors.size(); ++i) {
        for (size_t j = i + 1; j < neighbors.size(); ++j) {
            int v = neighbors[i];
            int w = neighbors[j];

            if (adjacencyMatrix[v][w]) {
                adjacencyMatrix[v][w] = 0;
                adjacencyMatrix[w][v] = 0;
            } else {
                adjacencyMatrix[v][w] = 1;
                adjacencyMatrix[w][v] = 1;
            }
        }
    }

    // Step 3: Update measurements
    switch (basis_u) {
        case MeasurementBasis::XY:
            measurements[u] = {MeasurementBasis::XZ, angle_u + M_PI / 2};
            break;
        case MeasurementBasis::XZ:
            measurements[u] = {MeasurementBasis::XY, M_PI / 2 - angle_u};
            break;
        case MeasurementBasis::YZ:
            measurements[u] = {MeasurementBasis::YZ, angle_u + M_PI / 2};
            break;
        case MeasurementBasis::X:
            measurements[u] = {MeasurementBasis::X, angle_u};
            break;
        case MeasurementBasis::Y:
            measurements[u] = {MeasurementBasis::Z, angle_u + M_PI};
            break;
        case MeasurementBasis::Z:
            measurements[u] = {MeasurementBasis::Y, angle_u};
            break;
        case MeasurementBasis::OUTPUT:
            outputAdjustments[u].adjustOutput("Rx", M_PI/2);
            break;
        default:
            break;
    }

    // Step 4: Update neighbors' measurements
    for (int v : neighbors) {
        auto [basis_v, angle_v] = getMeasurement(v);
        switch (basis_v) {
            case MeasurementBasis::XY:
                measurements[v] = {MeasurementBasis::XY, angle_v + M_PI / 2};
                break;
            case MeasurementBasis::XZ:
                measurements[v] = {MeasurementBasis::YZ, angle_v};
                break;
            case MeasurementBasis::YZ:
                measurements[v] = {MeasurementBasis::XZ, -angle_v};
                break;
            case MeasurementBasis::X:
                measurements[v] = {MeasurementBasis::Y, angle_v};
                break;
            case MeasurementBasis::Y:
                measurements[v] = {MeasurementBasis::X, angle_v + M_PI};
                break;
            case MeasurementBasis::OUTPUT:
                outputAdjustments[v].adjustOutput("Rz", -M_PI/2);
                break;
            default: // Z
                break;
        }
    }
}


void MBQC_Graph::pivot(int u, int v) {
    if (u < 0 || u >= size || v < 0 || v >= size) {
        std::cerr << "Invalid nodes for pivot: " << u << ", " << v << "\n";
        return;
    }

    if (!adjacencyMatrix[u][v]) {
        std::cerr << "Cannot pivot on non-adjacent nodes: " << u << ", " << v << "\n";
        return;
    }

    // Perform pivot: LC(u) -> LC(v) -> LC(u)
    localComplementation(u);
    localComplementation(v);
    localComplementation(u);

}


void MBQC_Graph::ZInsertion(const std::vector<int>& vertices) {
    
    int newNodeIndex = size;
    size += 1;
    measurements[newNodeIndex] = std::make_pair(MeasurementBasis::Z, 0.0);
    
    // Resize adjacency matrix
    adjacencyMatrix.resize(size);
    for (auto& row : adjacencyMatrix) {
        row.resize(size, 0);
    }
    // Add edge for all given vertices
    for (int v : vertices) {
        if (v >= 0 && v < size - 1) { 
            adjacencyMatrix[newNodeIndex][v] = 1;
            adjacencyMatrix[v][newNodeIndex] = 1;
        } else {
            std::cerr << "Warning: vertex " << v << " is out of range for ZInsertion\n";
        }
    }
}


void MBQC_Graph::ZDeletion(int u) {
    if (u < 0 || u >= size) {
        std::cerr << "ZDeletion: node " << u << " is out of range\n";
        return;
    }

    for (int v : outputs) {
        if (v == u) {
            std::cerr << "Z-Elimination not possible for output node " << v << ".\n";
            return;
        }
    }

    auto [basis_u, angle_u] = getMeasurement(u);
    
    std::set<MeasurementBasis> possible_bases = {MeasurementBasis::Z, MeasurementBasis::XZ, MeasurementBasis::YZ};
    if (!possible_bases.count(measurements[u].first)) {
        std::cerr << "ZDeletion: node " << u << " is not in Z, XZ or YZ basis\n";
        return;
    }
    
    angle_u = normalize_radians(angle_u);
    if (!fAlmostEqual(angle_u, 0) &&
        !fAlmostEqual(angle_u, M_PI)) {
        std::cerr << "ZDeletion: node " << u << " has angle " << radiansToString(angle_u) << " and thus not 0 or \u03c0!\n";
        return;
    }
    int a = static_cast<int>(std::round(angle_u / M_PI));

    // Get neighbors
    std::vector<int> neighbors;
    for (int v = 0; v < size; ++v) {
        if (adjacencyMatrix[u][v]) {
            neighbors.push_back(v);
        }
    }

    // Update neighbor's measurements
    for (int v : neighbors) {
        auto [basis_v, angle_v] = getMeasurement(v);
        switch (basis_v) {
            case MeasurementBasis::XY:
                measurements[v] = {MeasurementBasis::XY, angle_v + a * M_PI};
                break;
            case MeasurementBasis::X:
                measurements[v] = {MeasurementBasis::X, angle_v + a * M_PI};
                break;
            case MeasurementBasis::Y:
                measurements[v] = {MeasurementBasis::Y, angle_v + a * M_PI};
                break;
            case MeasurementBasis::XZ:
                measurements[v] = {MeasurementBasis::XZ, pow(-1, a) * angle_v};
                break;
            case MeasurementBasis::YZ:
                measurements[v] = {MeasurementBasis::YZ, pow(-1, a) * angle_v};
                break;
            case MeasurementBasis::OUTPUT:
                if (a) {
                    outputAdjustments[v].adjustOutput("Z");
                }
                break;
            default: // Z
                break;
        }
    }

    // Remove the row and column u from adjacencyMatrix
    adjacencyMatrix.erase(adjacencyMatrix.begin() + u);
    for (auto& row : adjacencyMatrix) {
        row.erase(row.begin() + u);
    }

    // Remove measurements for node u
    measurements.erase(u);

    // Decrement keys in measurements for nodes > u
    std::map<int, std::pair<MeasurementBasis, double>> newMeasurements;
    for (const auto& [node, data] : measurements) {
        if (node > u) {
            newMeasurements[node - 1] = data;
        } else if (node < u) {
            newMeasurements[node] = data;
        }
    }
    measurements = std::move(newMeasurements);

    // Decrement keys in outputAdjustments
    std::map<int, OutputAdjustmentMap> newOutputAdjustments;
    for (const auto& [node, outAdj] : outputAdjustments) {
        if (node > u) {
            newOutputAdjustments[node - 1] = outAdj;
        } else if (node < u) {
            newOutputAdjustments[node] = outAdj;
        }
    }
    outputAdjustments = std::move(newOutputAdjustments);

    // Adjust inputs vector
    for (auto it = inputs.begin(); it != inputs.end();) {
        if (*it == u) {
            it = inputs.erase(it);
        } else {
            if (*it > u) {
                *it = *it - 1;
            }
            ++it;
        }
    }

    // Adjust outputs vector
    for (auto it = outputs.begin(); it != outputs.end();) {
        if (*it == u) {
            it = outputs.erase(it);
        } else {
            if (*it > u) {
                *it = *it - 1;
            }
            ++it;
        }
    }

    // Decrement graph size
    size--;
}

void MBQC_Graph::ZDeletion(std::vector<int> nodes) {
    // Sort descending so that earlier deletions don't invalidate later indices
    std::sort(nodes.begin(), nodes.end(), std::greater<int>());

    for (int u : nodes) {
        ZDeletion(u);
    }
}

void MBQC_Graph::relabel(int u) {
    if (u < 0 || u >= size) {
        std::cerr << "Relabeling: node " << u << " is out of range\n";
        return;
    }

    auto [basis_u, angle_u] = getMeasurement(u);
    
    std::set<MeasurementBasis> possible_bases = {MeasurementBasis::XY, MeasurementBasis::XZ, MeasurementBasis::YZ};
    if (!possible_bases.count(measurements[u].first)) {
        std::cerr << "Relabeling: node " << u << " is not in XY, XZ or YZ basis\n";
        return;
    }
    
    angle_u = normalize_radians(angle_u);
    if (!(fAlmostEqual(angle_u, 0) ||
          fAlmostEqual(angle_u, M_PI/2) ||
          fAlmostEqual(angle_u, M_PI) ||
          fAlmostEqual(angle_u, 3*M_PI/2))) {
        std::cerr << "Relabeling: node " << u << " has angle " << radiansToString(angle_u) << " and thus not 0, pi/2, pi or 3pi/2\n";
        return;
    }

    switch (basis_u) {
        case MeasurementBasis::XY:
            if (fAlmostEqual(angle_u, 0) || fAlmostEqual(angle_u, M_PI)) {
                measurements[u] = {MeasurementBasis::X, angle_u};
            } else {
                measurements[u] = {MeasurementBasis::Y, angle_u - M_PI / 2};
            }
            break;
        case MeasurementBasis::XZ:
            if (fAlmostEqual(angle_u, 0) || fAlmostEqual(angle_u, M_PI)) {
                measurements[u] = {MeasurementBasis::Z, angle_u};
            } else {
                measurements[u] = {MeasurementBasis::X, angle_u - M_PI / 2};
            }
            break;
        case MeasurementBasis::YZ:
            if (fAlmostEqual(angle_u, 0) || fAlmostEqual(angle_u, M_PI)) {
                measurements[u] = {MeasurementBasis::Z, angle_u};
            } else {
                measurements[u] = {MeasurementBasis::Y, angle_u - M_PI / 2};
            }
            break;
        default:
            break;
    }
}

void MBQC_Graph::relabelPlanar(int u, MeasurementBasis preferredBasis) {
    if (u < 0 || u >= size) {
        std::cerr << "Relabeling to planar: node " << u << " is out of range\n";
        return;
    }

    auto [basis_u, angle_u] = getMeasurement(u);

    if (basis_u != MeasurementBasis::X &&
        basis_u != MeasurementBasis::Y &&
        basis_u != MeasurementBasis::Z) {
        std::cerr << "Relabeling to planar: node " << u << " is not in X, Y, or Z basis\n";
        return;
    }

    double new_angle;

    switch (preferredBasis) {
        case MeasurementBasis::XY:
            if (basis_u == MeasurementBasis::X)
                new_angle = angle_u;
            else if (basis_u == MeasurementBasis::Y)
                new_angle = angle_u + M_PI / 2;
            else {
                std::cerr << "Relabeling to planar: cannot express Z in XY plane\n";
                return;
            }
            break;

        case MeasurementBasis::XZ:
            if (basis_u == MeasurementBasis::Z)
                new_angle = angle_u;
            else if (basis_u == MeasurementBasis::X)
                new_angle = angle_u + M_PI / 2;
            else {
                std::cerr << "Relabeling to planar: cannot express Y in XZ plane\n";
                return;
            }
            break;

        case MeasurementBasis::YZ:
            if (basis_u == MeasurementBasis::Z)
                new_angle = angle_u;
            else if (basis_u == MeasurementBasis::Y)
                new_angle = angle_u + M_PI / 2;
            else {
                std::cerr << "Relabeling to planar: cannot express X in YZ plane\n";
                return;
            }
            break;

        default:
            std::cerr << "Relabeling to planar: unsupported preferred basis\n";
            return;
    }

    measurements[u] = {preferredBasis, new_angle};
}


void MBQC_Graph::relabelPlanar(int u) {
    auto [basis_u, angle_u] = getMeasurement(u);
    switch (basis_u) {
        case MeasurementBasis::X:
            relabelPlanar(u, MeasurementBasis::XZ);
            break;
        case MeasurementBasis::Y:
            relabelPlanar(u, MeasurementBasis::YZ);
            break;
        case MeasurementBasis::Z:
            relabelPlanar(u, MeasurementBasis::XZ);
            break;
        default:
            std::cerr << "Relabeling to planar: unsupported or already planar basis\n";
            break;
    }
}

// Merges two YZ nodes by summing their phases (https://doi.org/10.1103/PhysRevA.102.022406.)
void MBQC_Graph::mergeYZ(int u, int v) {
 
    if (u < 0 || u >= size || v < 0 || v >= size) {
        std::cerr << "mergeYZNodes: node out of range\n";
    }
    if (isOutput(u) || isOutput(v)) {
        std::cerr << "mergeYZNodes: cannot merge output nodes\n";
    }
 
    auto [basis_u, angle_u] = getMeasurement(u);
    auto [basis_v, angle_v] = getMeasurement(v);
 
    if (basis_u != MeasurementBasis::YZ || basis_v != MeasurementBasis::YZ) {
        std::cerr << "mergeYZNodes: both nodes must be in YZ basis\n";
    }
    if (adjacencyMatrix[u][v]) {
        std::cerr << "mergeYZNodes: nodes " << u << " and " << v << " are neighbors, cannot merge\n";
    }
 
    std::vector<int> neighbors_u = getNeighbors(u);
    std::vector<int> neighbors_v = getNeighbors(v);
 
    std::set<int> set_u(neighbors_u.begin(), neighbors_u.end());
    std::set<int> set_v(neighbors_v.begin(), neighbors_v.end());
 
    if (set_u != set_v) {
        std::cerr << "mergeYZNodes: nodes " << u << " and " << v << " do not share the same neighbors\n";
    }
 
    // Merge: keep u, delete v
    double mergedAngle = normalize_radians(angle_u + angle_v);
    measurements[u] = {MeasurementBasis::YZ, mergedAngle};
 
    // Delete v (use single-node ZDeletion-style removal)
    adjacencyMatrix.erase(adjacencyMatrix.begin() + v);
    for (auto& row : adjacencyMatrix) {
        row.erase(row.begin() + v);
    }
    measurements.erase(v);
    std::map<int, std::pair<MeasurementBasis, double>> newMeasurements;
    for (const auto& [node, data] : measurements) {
        newMeasurements[node > v ? node - 1 : node] = data;
    }
    measurements = std::move(newMeasurements);
    std::map<int, OutputAdjustmentMap> newOutputAdjustments;
    for (const auto& [node, oa] : outputAdjustments) {
        newOutputAdjustments[node > v ? node - 1 : node] = oa;
    }
    outputAdjustments = std::move(newOutputAdjustments);
    for (auto& x : inputs)  if (x > v) --x;
    for (auto& x : outputs) if (x > v) --x;
    // If u was above v in index, its index shifted down by 1
    if (u > v) --u;
    size--;
}


// Adds a new YZ vertex to a vertex u in the XY plane, connected only to u. The new angle of u
// becomes β, and the new YZ vertex's angle becomes β-ɑ, where ɑ is u's old angle.
void MBQC_Graph::YZUnfusion(int u, double beta) {
    if (u < 0 || u >= size) {
        std::cerr << "YZUnfusion: node " << u << " is out of range\n";
        return;
    }

    auto [basis_u, angle_u] = getMeasurement(u);
    if (basis_u != MeasurementBasis::XY) {
        std::cerr << "YZUnfusion: node " << u << " is not in XY basis\n";
        return;
    }

    beta = normalize_radians(beta);

    int newNodeIndex = size;
    size += 1;

    adjacencyMatrix.resize(size);
    for (auto& row : adjacencyMatrix) {
        row.resize(size, 0);
    }
    adjacencyMatrix[newNodeIndex][u] = 1;
    adjacencyMatrix[u][newNodeIndex] = 1;

    measurements[u] = {MeasurementBasis::XY, beta};
    measurements[newNodeIndex] = {MeasurementBasis::YZ, normalize_radians(beta - angle_u)};
}


// ########## AUTOMATIC SIMPLIFICATION ##############

void MBQC_Graph::simplify(int maxIterations) {
 
    std::unordered_set<std::string> seenStates;
    int iterations = 0;
 
    while (true) {
 
        // Break condition 1: max iterations
        if (iterations >= maxIterations) {
            std::cerr << "simplify(): reached max iterations (" << maxIterations << "), stopping.\n";
            break;
        }
 
        // Break condition 2: repetition detection via state hash
        std::string hash = stateHash();
        if (seenStates.count(hash)) {
            break;
        }
        seenStates.insert(hash);
        ++iterations;
 
        // Relabel all eligible nodes to Pauli basis 
        for (const auto& [node, data] : measurements) {
            MeasurementBasis basis = data.first;
            double angle = normalize_radians(data.second);
 
            bool isPlanar = (basis == MeasurementBasis::XY ||
                             basis == MeasurementBasis::XZ ||
                             basis == MeasurementBasis::YZ);
            bool isQuarterAngle = fAlmostEqual(fmod(angle, M_PI / 2), 0);
 
            if (isPlanar && isQuarterAngle && !isOutput(node)) {
                relabel(node);
            }
        }
 
        // Local complementation on all Y nodes
        std::vector<int> yNodes;
        for (const auto& [node, data] : measurements) {
            if (data.first == MeasurementBasis::Y && !isOutput(node) && !isInput(node))
                yNodes.push_back(node);
        }
        for (int node : yNodes) {
            // Re-check: LC on a Y node may have changed a neighbour's basis
            if (measurements.count(node) && measurements.at(node).first == MeasurementBasis::Y) {
                localComplementation(node);
            }
        }
 
        // Pivot on all X nodes that have a non-input neighbor
        std::vector<int> xNodes;
        for (const auto& [node, data] : measurements) {
            if (data.first == MeasurementBasis::X && !isInput(node))
                xNodes.push_back(node);
        }
        for (int node : xNodes) {
            if (measurements.at(node).first != MeasurementBasis::X) continue; // may have been removed by earlier pivot
 
            int pivotNeighbor = -1;
            for (int nb : getNeighbors(node)) {
                if (!isInput(nb)) { pivotNeighbor = nb; break; }
            }
 
            if (pivotNeighbor != -1) {
                pivot(node, pivotNeighbor);
            }
        }
 
        // ZDeletion on all eligible Z nodes
        std::vector<int> zCandidates;
        for (const auto& [node, data] : measurements) {
            MeasurementBasis basis = data.first;
            double angle = normalize_radians(data.second);
 
            bool isZBasis = (basis == MeasurementBasis::Z ||
                             basis == MeasurementBasis::XZ ||
                             basis == MeasurementBasis::YZ);
            bool isValidAngle = fAlmostEqual(angle, 0) || fAlmostEqual(angle, M_PI);
 
            if (isZBasis && isValidAngle && !isOutput(node)) {
                zCandidates.push_back(node);
            }
        }
        if (!zCandidates.empty()) {
            ZDeletion(zCandidates);
        }

        // YZ node merger
        mergeAllYZNodes();
 
    }
}


// Scans all pairs of YZ nodes and merges any eligible pair, and absorbs any YZ node whose only
// neighbor is an XY node back into that neighbor.
// Returns true if at least one merge/absorption was performed.
bool MBQC_Graph::mergeAllYZNodes() {
    bool anyMerged = false;

    bool merged = true;
    while (merged) {
        merged = false;

        // Collect current YZ non-output nodes
        std::vector<int> yzNodes;
        for (const auto& [node, data] : measurements) {
            if (data.first == MeasurementBasis::YZ && !isOutput(node))
                yzNodes.push_back(node);
        }

        // Absorb YZ pendants: undoes YZUnfusion. A YZ node w with a single XY neighbor u
        // (angle A) can always be phase-shifted to angle 0 by moving its own angle onto u
        // (u's new angle = A - w's angle), since shifting both u and w by the same amount
        // preserves u.angle - w.angle, the invariant that ties the pair to the same
        // pre-unfusion state. Once w sits at angle 0 it's a no-op for ZDeletion's neighbors,
        // so ZDeletion(w) just removes it structurally.
        for (int w : yzNodes) {
            std::vector<int> neighbors = getNeighbors(w);
            if (neighbors.size() != 1) continue;

            int u = neighbors[0];
            auto [basisU, angleU] = getMeasurement(u);
            if (basisU != MeasurementBasis::XY) continue;

            auto [basisW, angleW] = getMeasurement(w);
            measurements[u] = {MeasurementBasis::XY, normalize_radians(angleU - angleW)};
            measurements[w] = {MeasurementBasis::YZ, 0.0};

            ZDeletion(w);
            merged = true;
            anyMerged = true;
            break; // ZDeletion(w) shifted indices; restart with a fresh scan
        }
        if (merged) continue;

        // Try all pairs
        for (size_t i = 0; i < yzNodes.size() && !merged; ++i) {
            for (size_t j = i + 1; j < yzNodes.size() && !merged; ++j) {
                int a = yzNodes[i];
                int b = yzNodes[j];
 
                if (adjacencyMatrix[a][b]) continue;
 
                std::vector<int> na = getNeighbors(a);
                std::vector<int> nb = getNeighbors(b);
                std::set<int> sa(na.begin(), na.end());
                std::set<int> sb(nb.begin(), nb.end());
 
                if (sa == sb) {
                    mergeYZ(a, b);
                    merged = true;
                    anyMerged = true;
                }
            }
        }
    }
 
    return anyMerged;
}


// Number of edges among neighborSubset (an induced-subgraph edge count), used by both
// lcompCost and pivotCost's boundary-edge counting.
static int inducedEdgeCount(const std::vector<std::vector<int>>& adjacencyMatrix, const std::vector<int>& nodes) {
    int count = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            if (adjacencyMatrix[nodes[i]][nodes[j]]) {
                ++count;
            }
        }
    }
    return count;
}


// Number of edges saved by performing a local complementation on v, restricted to
// neighborSubset (v's ordinary full neighborhood, or a partial "nu-set"). Positive means the
// operation strictly reduces the total edge count of the graph. Mirrors lcomp_cost() in
// pattern_optimize.py: neighborSubset != getNeighbors(v) incurs an "unfusion" penalty, since
// realizing it requires inserting a helper vertex (see lcompRewrite).
int MBQC_Graph::lcompCost(int v, const std::vector<int>& neighborSubset) const {
    int n = static_cast<int>(neighborSubset.size());
    int edgesInSubset = inducedEdgeCount(adjacencyMatrix, neighborSubset);
    int maxEdges = n * (n - 1) / 2;

    std::vector<int> fullNeighbors = getNeighbors(v);
    std::set<int> fullSet(fullNeighbors.begin(), fullNeighbors.end());
    std::set<int> subsetSet(neighborSubset.begin(), neighborSubset.end());
    bool isFullNeighborhood = (subsetSet == fullSet);
    int unfusion = isFullNeighborhood ? 0 : -1;

    // If v is measured in the Y basis, local complementation turns it into a Z-basis node
    // that can then be Z-deleted, additionally removing all n of its incident edges. Only
    // applies to the ordinary (non-unfused) full-neighborhood complementation.
    auto [basis_v, angle_v] = getMeasurement(v);
    int zBonus = (basis_v == MeasurementBasis::Y && isFullNeighborhood && !isOutput(v)) ? n : 0;

    return 2 * edgesInSubset - maxEdges + zBonus + unfusion;
}


// Number of edges saved by performing a pivot on edge (u,v), restricted to candidate neighbor
// subsets neighborsU/neighborsV of u and v. Mirrors pivot_cost() in pattern_optimize.py.
int MBQC_Graph::pivotCost(int u, int v, const std::vector<int>& neighborsU, const std::vector<int>& neighborsV) const {
    std::set<int> setU(neighborsU.begin(), neighborsU.end());
    std::set<int> setV(neighborsV.begin(), neighborsV.end());

    std::set<int> A, B, C;
    for (int x : setU) if (!setV.count(x) && x != v) A.insert(x);
    for (int x : setV) if (!setU.count(x) && x != u) B.insert(x);
    for (int x : setU) if (setV.count(x)) C.insert(x);

    int maxEdges = static_cast<int>(A.size()) * static_cast<int>(B.size())
                 + static_cast<int>(A.size()) * static_cast<int>(C.size())
                 + static_cast<int>(B.size()) * static_cast<int>(C.size());

    std::vector<int> fullU = getNeighbors(u);
    std::vector<int> fullV = getNeighbors(v);
    std::set<int> fullSetU(fullU.begin(), fullU.end());
    std::set<int> fullSetV(fullV.begin(), fullV.end());
    bool fullNeighborhoodU = (setU == fullSetU);
    bool fullNeighborhoodV = (setV == fullSetV);
    int unfusionU = fullNeighborhoodU ? 0 : -1;
    int unfusionV = fullNeighborhoodV ? 0 : -1;

    auto edgeBoundary = [this](const std::set<int>& s1, const std::set<int>& s2) {
        int count = 0;
        for (int a : s1) {
            for (int b : s2) {
                if (adjacencyMatrix[a][b]) ++count;
            }
        }
        return count;
    };
    int numEdges = edgeBoundary(A, B) + edgeBoundary(A, C) + edgeBoundary(B, C);

    auto [basis_u, angle_u] = getMeasurement(u);
    auto [basis_v, angle_v] = getMeasurement(v);

    int zBonusU = (basis_u == MeasurementBasis::X && fullNeighborhoodU && !isOutput(u)) ? static_cast<int>(fullV.size()) : 0;
    int zBonusV = (basis_v == MeasurementBasis::X && fullNeighborhoodV && !isOutput(v)) ? static_cast<int>(fullU.size()) : 0;
    if (zBonusU > 0 && zBonusV > 0) {
        zBonusV -= 1;
    }

    return 2 * numEdges - maxEdges + zBonusU + zBonusV + unfusionU + unfusionV;
}


// Greedily grows a nu-set (partial neighborhood of v) that locally maximizes lcompCost(v, .),
// starting from the highest-degree vertex of the 2-core of v's neighborhood subgraph and
// expanding along that subgraph's edges. Mirrors find_best_lcomp_nu_set() in
// pattern_optimize.py. Returns ({}, 0) if no productive nu-set is found.
std::pair<std::vector<int>, int> MBQC_Graph::findBestLcompNuSet(int v) const {
    std::vector<int> neighborsV = getNeighbors(v);
    std::set<int> H(neighborsV.begin(), neighborsV.end());

    auto degreeInH = [this, &H](int w) {
        int d = 0;
        for (int x : H) {
            if (x != w && adjacencyMatrix[w][x]) ++d;
        }
        return d;
    };

    // Repeatedly strip nodes of degree < 2 within H (the 2-core of the neighborhood subgraph).
    while (true) {
        std::vector<int> toRemove;
        for (int w : H) {
            if (degreeInH(w) < 2) toRemove.push_back(w);
        }
        if (toRemove.empty()) break;
        for (int w : toRemove) H.erase(w);
    }

    if (H.empty()) {
        return {std::vector<int>(), 0};
    }

    int maxDegreeNode = -1, maxDeg = -1;
    for (int w : H) {
        int d = degreeInH(w);
        if (d > maxDeg) { maxDeg = d; maxDegreeNode = w; }
    }

    std::set<int> nuSet = {maxDegreeNode};
    std::set<int> openNeighbors;
    for (int x : H) {
        if (x != maxDegreeNode && adjacencyMatrix[maxDegreeNode][x]) openNeighbors.insert(x);
    }

    int fVal = lcompCost(v, std::vector<int>(nuSet.begin(), nuSet.end()));

    while (nuSet.size() < H.size() && !openNeighbors.empty()) {
        int bestN = -1, bestScore = std::numeric_limits<int>::min();
        for (int n : openNeighbors) {
            std::vector<int> candidate(nuSet.begin(), nuSet.end());
            candidate.push_back(n);
            int score = lcompCost(v, candidate);
            if (score > bestScore) { bestScore = score; bestN = n; }
        }

        if (bestScore >= fVal) {
            nuSet.insert(bestN);
            openNeighbors.erase(bestN);
            for (int x : H) {
                if (x != bestN && adjacencyMatrix[bestN][x] && !nuSet.count(x)) {
                    openNeighbors.insert(x);
                }
            }
            fVal = bestScore;
        } else {
            break;
        }
    }

    return {std::vector<int>(nuSet.begin(), nuSet.end()), fVal};
}


// Greedily grows nu-sets (partial neighborhoods of u and v) that locally maximize
// pivotCost(u, v, .), by expanding along the cross-boundary edges among
// A = N(u)\N(v)\{v}, B = N(v)\N(u)\{u}, C = N(u)\N(v). Mirrors find_best_pivot_nu_sets() in
// pattern_optimize.py. Returns ({{}, {}}, 0) if no productive nu-sets are found.
std::pair<std::pair<std::vector<int>, std::vector<int>>, int> MBQC_Graph::findBestPivotNuSets(int u, int v) const {
    std::vector<int> neighborsU = getNeighbors(u);
    std::vector<int> neighborsV = getNeighbors(v);
    std::set<int> setU(neighborsU.begin(), neighborsU.end());
    std::set<int> setV(neighborsV.begin(), neighborsV.end());

    std::set<int> A, B, C;
    for (int x : setU) if (!setV.count(x) && x != v) A.insert(x);
    for (int x : setV) if (!setU.count(x) && x != u) B.insert(x);
    for (int x : setU) if (setV.count(x)) C.insert(x);

    auto groupOf = [&](int x) {
        if (A.count(x)) return 0;
        if (B.count(x)) return 1;
        return 2; // C
    };

    std::set<int> allNodes;
    allNodes.insert(A.begin(), A.end());
    allNodes.insert(B.begin(), B.end());
    allNodes.insert(C.begin(), C.end());

    // H keeps only cross-group edges (A-B, A-C, B-C); in-group edges don't affect pivotCost.
    std::map<int, std::set<int>> Hadj;
    for (int x : allNodes) Hadj[x] = {};
    for (int x : allNodes) {
        for (int y : allNodes) {
            if (x < y && adjacencyMatrix[x][y] && groupOf(x) != groupOf(y)) {
                Hadj[x].insert(y);
                Hadj[y].insert(x);
            }
        }
    }

    std::set<int> Hnodes;
    for (int x : allNodes) {
        if (!Hadj[x].empty()) Hnodes.insert(x);
    }

    if (Hnodes.empty()) {
        return {{std::vector<int>(), std::vector<int>()}, 0};
    }

    int maxDegNode = -1, maxDeg = -1;
    for (int x : Hnodes) {
        int d = static_cast<int>(Hadj[x].size());
        if (d > maxDeg) { maxDeg = d; maxDegNode = x; }
    }

    std::set<int> nuSet = {maxDegNode};
    std::set<int> openNeighbors = Hadj[maxDegNode];

    auto scoreOf = [&](const std::set<int>& candidateSet) {
        std::vector<int> nuU, nuV;
        for (int x : candidateSet) {
            if (setU.count(x)) nuU.push_back(x);
            if (setV.count(x)) nuV.push_back(x);
        }
        return pivotCost(u, v, nuU, nuV);
    };

    int fVal = scoreOf(nuSet);

    while (!openNeighbors.empty()) {
        int bestN = -1, bestScore = std::numeric_limits<int>::min();
        for (int n : openNeighbors) {
            std::set<int> candidate = nuSet;
            candidate.insert(n);
            int score = scoreOf(candidate);
            if (score > bestScore) { bestScore = score; bestN = n; }
        }

        if (bestScore >= fVal) {
            nuSet.insert(bestN);
            openNeighbors.erase(bestN);
            for (int x : Hadj[bestN]) {
                if (!nuSet.count(x)) openNeighbors.insert(x);
            }
            fVal = bestScore;
        } else {
            break;
        }
    }

    std::vector<int> finalNuU, finalNuV;
    for (int x : nuSet) {
        if (setU.count(x)) finalNuU.push_back(x);
        if (setV.count(x)) finalNuV.push_back(x);
    }

    return {{finalNuU, finalNuV}, fVal};
}


// Mirrors rule_with_z_deletion() in pattern_optimize.py: whether applying `rule` here would
// expose a Pauli-basis node that can subsequently be Z-deleted, used to justify applying an
// otherwise zero-score rewrite (it still shrinks the graph, just not its edge count alone).
bool MBQC_Graph::ruleFavorsZDeletion(bool isPivot, int u, int v) const {
    if (isPivot) {
        auto [basisU, angleU] = getMeasurement(u);
        auto [basisV, angleV] = getMeasurement(v);
        return basisU == MeasurementBasis::X || basisV == MeasurementBasis::X;
    }
    auto [basisU, angleU] = getMeasurement(u);
    return basisU == MeasurementBasis::Y;
}


// Local complementation restricted to neighborSubset. Mirrors lcomp_rewrite() in
// pattern_optimize.py.
int MBQC_Graph::lcompRewrite(int u, const std::vector<int>& neighborSubset) {
    std::vector<int> fullNeighbors = getNeighbors(u);
    std::set<int> fullSet(fullNeighbors.begin(), fullNeighbors.end());
    std::set<int> subsetSet(neighborSubset.begin(), neighborSubset.end());

    if (fullSet == subsetSet) {
        localComplementation(u);
        return u;
    }

    std::vector<int> insertNeighbors = neighborSubset;
    insertNeighbors.push_back(u);
    ZInsertion(insertNeighbors);
    int helper = size - 1;
    localComplementation(helper);
    return helper;
}


// Pivot on (u,v) restricted to candidate neighbor subsets neighborsU/neighborsV, via three
// lcompRewrite calls (each of which may unfuse its target vertex). Mirrors pivot_rewrite() in
// pattern_optimize.py. When neighborsU/neighborsV are u's/v's actual full neighborhoods, this
// reduces to the ordinary three-fold pivot() = LC(u); LC(v); LC(u).
std::pair<int, int> MBQC_Graph::pivotRewrite(int u, int v, const std::vector<int>& neighborsU, const std::vector<int>& neighborsV) {
    std::set<int> setU(neighborsU.begin(), neighborsU.end());
    std::set<int> setV(neighborsV.begin(), neighborsV.end());

    std::set<int> A, B, C;
    for (int x : setU) if (!setV.count(x) && x != v) A.insert(x);
    for (int x : setV) if (!setU.count(x) && x != u) B.insert(x);
    for (int x : setU) if (setV.count(x)) C.insert(x);

    std::vector<int> firstSet(A.begin(), A.end());
    firstSet.insert(firstSet.end(), C.begin(), C.end());
    firstSet.push_back(v);
    int newU = lcompRewrite(u, firstSet);

    std::vector<int> secondSet(A.begin(), A.end());
    secondSet.insert(secondSet.end(), B.begin(), B.end());
    secondSet.push_back(newU);
    int newV = lcompRewrite(v, secondSet);

    std::vector<int> thirdSet(B.begin(), B.end());
    thirdSet.insert(thirdSet.end(), C.begin(), C.end());
    thirdSet.push_back(newV);
    int newU2 = lcompRewrite(newU, thirdSet);

    if (newU != newU2) {
        auto [basisNewU, angleNewU] = getMeasurement(newU);
        if (basisNewU == MeasurementBasis::Y) {
            localComplementation(newU);
            ZDeletion(newU);
            // ZDeletion(newU) shifts down every index greater than newU.
            if (newU2 > newU) --newU2;
            if (newV > newU) --newV;
        }
    }

    return {newU2, newV};
}


// Greedy edge-count reduction that alternates between local complementation and pivot,
// each considered both on full neighborhoods and on greedily-searched partial "nu-set"
// neighborhoods (via vertex unfusion). At each step, applies whichever candidate rewrite has
// the highest score; when favorVertexRemoval is set, a zero-score rewrite is still applied if
// it exposes a node for ZDeletion. Stops once no candidate is worth applying. Mirrors
// greedy_optimize_edges() in pattern_optimize.py.
std::vector<GraphRewriteStep> MBQC_Graph::greedyOptimizeEdges(bool favorVertexRemoval) {
    struct Candidate {
        bool isPivot;
        int u, v;
        std::vector<int> nuU, nuV;
        int score;
    };

    std::vector<GraphRewriteStep> rules;

    // Outer loop: mergeAllYZNodes() (YZ-pendant absorption / pair merging) can change node
    // angles or remove vertices, which may expose new LC/pivot opportunities that the inner
    // search already converged past. Keep alternating until neither phase makes progress, so a
    // single call reaches a true fixed point (a second call finds nothing left to do).
    bool mergedAnything = true;
    while (mergedAnything) {
        while (true) {
            // Relabel eligible planar nodes to Pauli basis (mirrors simplify()'s first step).
            // Without this, e.g. a Clifford XY(0) node stays labeled XY forever and lcompCost's/
            // pivotCost's X/Y-basis z_bonus (and thus most real reduction opportunities on
            // circuit-derived graphs, which are built entirely out of XY-labeled nodes) never
            // triggers.
            for (const auto& [node, data] : measurements) {
                MeasurementBasis basis = data.first;
                double angle = normalize_radians(data.second);

                bool isPlanar = (basis == MeasurementBasis::XY ||
                                 basis == MeasurementBasis::XZ ||
                                 basis == MeasurementBasis::YZ);
                bool isQuarterAngle = fAlmostEqual(fmod(angle, M_PI / 2), 0);

                if (isPlanar && isQuarterAngle && !isOutput(node)) {
                    relabel(node);
                }
            }

            std::vector<Candidate> candidates;
            std::vector<int> nonInputs = getNonInputs();

            std::vector<std::pair<int, int>> edges;
            for (auto& [a, b] : getAllEdges()) {
                if (a < b && !isInput(a) && !isInput(b)) {
                    edges.push_back({a, b});
                }
            }

            for (int node : nonInputs) {
                std::vector<int> fullN = getNeighbors(node);
                candidates.push_back({false, node, -1, fullN, {}, lcompCost(node, fullN)});
            }
            for (auto& [a, b] : edges) {
                std::vector<int> fullA = getNeighbors(a);
                std::vector<int> fullB = getNeighbors(b);
                candidates.push_back({true, a, b, fullA, fullB, pivotCost(a, b, fullA, fullB)});
            }
            for (int node : nonInputs) {
                auto [nuSet, score] = findBestLcompNuSet(node);
                candidates.push_back({false, node, -1, nuSet, {}, score});
            }
            for (auto& [a, b] : edges) {
                auto [nuSets, score] = findBestPivotNuSets(a, b);
                candidates.push_back({true, a, b, nuSets.first, nuSets.second, score});
            }

            if (candidates.empty()) break;

            size_t bestIdx = 0;
            for (size_t i = 1; i < candidates.size(); ++i) {
                if (candidates[i].score > candidates[bestIdx].score) bestIdx = i;
            }
            Candidate best = candidates[bestIdx];

            bool apply = best.score > 0 ||
                         (best.score == 0 && favorVertexRemoval && ruleFavorsZDeletion(best.isPivot, best.u, best.v));
            if (!apply) break;

            auto zDeleteIfPauliZ = [this](int node) {
                auto [basis, angle] = getMeasurement(node);
                double normAngle = normalize_radians(angle);
                if (basis == MeasurementBasis::Z && !isOutput(node) &&
                    (fAlmostEqual(normAngle, 0) || fAlmostEqual(normAngle, M_PI))) {
                    ZDeletion(node);
                }
            };

            if (best.isPivot) {
                auto [newU2, newV] = pivotRewrite(best.u, best.v, best.nuU, best.nuV);
                std::vector<int> toCheck = {newU2, newV};
                std::sort(toCheck.begin(), toCheck.end(), std::greater<int>());
                for (int node : toCheck) zDeleteIfPauliZ(node);
            } else {
                int newV = lcompRewrite(best.u, best.nuU);
                zDeleteIfPauliZ(newV);
            }

            rules.push_back({best.isPivot ? GraphRewriteRuleType::Pivot : GraphRewriteRuleType::LocalComplementation,
                              best.u, best.v, best.score});
        }

        // Clean up any YZ-pendant-on-XY structures left behind by the nu-set unfusions above
        // (lcompRewrite/pivotRewrite), and merge any now-mergeable YZ pairs. If that changed
        // anything, loop back: it may have exposed new LC/pivot opportunities.
        mergedAnything = mergeAllYZNodes();
    }

    return rules;
}


bool MBQC_Graph::canOptimizeEdges(bool favorVertexRemoval) const {
    MBQC_Graph probe = clone();
    return !probe.greedyOptimizeEdges(favorVertexRemoval).empty();
}



// ########## JSON ##############
using json = nlohmann::json;


json MBQC_Graph::toJson() const {
    json j;

    j["size"] = size;
    j["inputs"] = inputs;
    j["outputs"] = outputs;

    std::vector<json> measVec(size, nullptr);
    for (const auto& [node, pair] : measurements) {
        std::string basisStr = basisToString(pair.first);
        std::string angleStr = radiansToString(pair.second);
        measVec[node] = { basisStr, angleStr };
    }
    j["meas"] = measVec;

    std::vector<std::pair<int, int>> edgeList;
    for (int u = 0; u < size; ++u) {
        for (int v = u + 1; v < size; ++v) {
            if (adjacencyMatrix[u][v] != 0) {
                edgeList.emplace_back(u, v);
            }
        }
    }
    j["edges"] = edgeList;

    json outputAdjustmentList;

    for (const auto& [qubitIndex, adjustment] : outputAdjustments) {
        outputAdjustmentList[std::to_string(qubitIndex)] = adjustment.toJson();
    }
    
    j["outAdj"] = outputAdjustmentList;

    return j;
}

MBQC_Graph MBQC_Graph::fromJson(const json& j) {
    int size = static_cast<int>(j.at("size"));
    std::vector<int> inputs = j.at("inputs").get<std::vector<int>>();
    std::vector<int> outputs = j.at("outputs").get<std::vector<int>>();

    MBQC_Graph graph(size, inputs, outputs);

    for (const auto& edge : j["edges"]) {
        int u = edge[0];
        int v = edge[1];
        graph.addEdge(u,v);
    }

    for (auto& [node_str, measData] : j["meas"].items()) {
        graph.setMeasurement(std::stoi(node_str), parseBasis(measData["basis"]), parseAngle(measData["angle"]));
    }

    for (auto& [node_str, adjustment] : j["outAdj"].items()) {
        graph.setOutputAdjustment(std::stoi(node_str), OutputAdjustmentMap::fromJson(adjustment));
    }

    return graph;
}







// ########## OLD PYZX JSON ##############

void MBQC_Graph::exportToPYZXJsonFile(const std::string& filename, int rowLength) const {
    json j;
    j["version"] = 2;
    j["backend"] = "simple";
    j["variable_types"] = json::object();
    j["scalar"] = {
        {"power2", 5},
        {"phase", "0"}
    };

    j["inputs"] = inputs;
    j["outputs"] = outputs;

    // Vertices
    json vertices = json::array();
    for (int i = 0; i < size; ++i) {
        json v;
        v["id"] = i;
        int t = 0;
        std::string phase = "";

        auto it = measurements.find(i);
        if (it != measurements.end()) {
            t = basis_to_t(it->second.first);
            phase = radiansToString(it->second.second);
        }

        v["t"] = t;
        v["pos"] = { i % rowLength, i / rowLength };
        if (!phase.empty()) {
            v["phase"] = phase;
        }
        vertices.push_back(v);
    }
    j["vertices"] = vertices;

    // Edges
    json edges = json::array();
    for (int u = 0; u < size; ++u) {
        for (int v = u + 1; v < size; ++v) {
            if (adjacencyMatrix[u][v]) {
                // Just use default edge type = 1
                edges.push_back({ u, v, 1 });
            }
        }
    }
    j["edges"] = edges;

    std::ofstream out(filename);
    out << j.dump(2);
    out.close();
}

MBQC_Graph MBQC_Graph::importFromPYZXJsonFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    json j;
    in >> j;

    // Read inputs and outputs
    std::vector<int> inputs = j["inputs"].get<std::vector<int>>();
    std::vector<int> outputs = j["outputs"].get<std::vector<int>>();

    // Determine number of vertices
    int numNodes = j["vertices"].size();
    MBQC_Graph newGraph(numNodes, inputs, outputs);

    // Read vertices
    for (const auto& v : j["vertices"]) {
        int id = v["id"];
        int t = v["t"];
        double angle = 0.0;

        if (v.contains("phase")) {
            std::string phaseStr = v["phase"];
            if (phaseStr == "\u03c0") angle = M_PI;
            else if (phaseStr == "2\u03c0") angle = 2 * M_PI;
            else if (phaseStr == "\u03c0/2") angle = M_PI / 2;
            else if (phaseStr == "3\u03c0/2") angle = 3 * M_PI / 2;
            else if (!phaseStr.empty()) angle = std::stod(phaseStr);
        }

        MeasurementBasis basis = MeasurementBasis::X; // Default
        switch (t) {
            case 1: basis = MeasurementBasis::X; break;
            case 2: basis = MeasurementBasis::Y; break;
            case 3: basis = MeasurementBasis::Z; break;
            case 4: basis = MeasurementBasis::XY; break;
            case 5: basis = MeasurementBasis::YZ; break;
            case 6: basis = MeasurementBasis::XZ; break;
            default: break;
        }

        newGraph.setMeasurement(id, basis, angle);
    }

    // Read edges
    for (const auto& edge : j["edges"]) {
        int u = edge[0];
        int v = edge[1];
        int t = edge[2];
        newGraph.addEdge(u, v);
    }

    return newGraph;
}
