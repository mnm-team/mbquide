#include "utils.hpp"
#include "MBQC_Graph.hpp"


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
    if (!fAlmostEqual(fmod(angle_u, M_PI/2), 0)) {
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




// ########## FLOW ##############

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

// Give all measuremnt vertices (now all non-outputs, because inputs have a measurement basis)
std::vector<int> MBQC_Graph::mvertices() const {
    return getNonOutputs();
}


// Flow-demand matrix (Definition 3.4 in http://arxiv.org/abs/2410.23439)
std::vector<std::vector<int>> MBQC_Graph::getFlowDemandMatrix() const {

    std::vector<int> nonOutputs = getNonOutputs();
    std::vector<int> nonInputs = getNonInputs();

    // Map vertex -> column index
    std::unordered_map<int, int> colIndex;
    for (int j = 0; j < nonInputs.size(); j++) {
        colIndex[nonInputs[j]] = j;
    }

    std::vector<std::vector<int>> flowMatrix(nonOutputs.size(),  // rows 
                                                std::vector<int>(nonInputs.size(), 0));  // columns

    for (int i = 0; i < (int)nonOutputs.size(); i++) {
        int v = nonOutputs[i];
        auto [basis, angle] = getMeasurement(v);

        // Case 1: λ(v) in {X, XY}
        if (basis == MeasurementBasis::X || basis == MeasurementBasis::XY) {
            for (int w : nonInputs) {
                flowMatrix[i][colIndex[w]] = adjacencyMatrix[v][w];
            }
        }
        // Case 2: λ(v) in {Z, YZ, XZ}
        else if (basis == MeasurementBasis::Z || basis == MeasurementBasis::YZ || basis == MeasurementBasis::XZ) {
            if (colIndex.count(v)) {  // Check if non-out v is also a non-in
                flowMatrix[i][colIndex.at(v)] = 1;
            }
        }
        // Case 3: λ(v) in {Y}
        else if (basis == MeasurementBasis::Y) {
            for (int w : nonInputs) {
                if (w == v) {
                    flowMatrix[i][colIndex.at(v)] = 1;
                    continue;
                }
                flowMatrix[i][colIndex[w]] = adjacencyMatrix[v][w];
            }
        }
    }

    return flowMatrix;
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
