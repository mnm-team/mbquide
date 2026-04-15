#include "ZX_Graph.hpp"
#include <cmath>


std::string spiderTypeToString(SpiderType s) {
    if (s == SpiderType::Z) {
        return "Z";
    } else if (s == SpiderType::X) {
        return "X";
    } else if (s == SpiderType::INPUT) {
        return "INPUT";
    } else if (s == SpiderType::OUTPUT) {
        return "OUTPUT";
    }
    std::cerr << "Cannot convert SpiderType to string!\n";
    return "";
}

int spiderTypeToT(SpiderType s) {
    if (s == SpiderType::Z) {
        return 1;
    } else if (s == SpiderType::X) {
        return 2;
    } else if (s == SpiderType::INPUT) {
        return 0;
    } else if (s == SpiderType::OUTPUT) {
        return 0;
    }
    std::cerr << "Cannot convert SpiderType to t!\n";
    return -1;
}



int edgeTypeToInt(EdgeType type) {
    if (type == EdgeType::SIMPLE) {
        return 1;
    } else if (type == EdgeType::HADAMARD) {
        return 2;
    } 
    std::cerr << "Cannot convert EdgeType to int!\n";
    return -1;
}


int ZXGraph::addSpider(SpiderType type, double phase) {
    int id = next_spider_id++;
    spiders[id] = {id, type, phase, {}};
    return id;
}

void ZXGraph::addEdge(int id1, int id2, EdgeType type) {
    spiders[id1].neighbors[id2] = type;
    spiders[id2].neighbors[id1] = type;
    edges.push_back(std::make_tuple(id1, id2, type));
}

void ZXGraph::removeEdge(int id1, int id2) {
    spiders[id1].neighbors.erase(id2);
    spiders[id2].neighbors.erase(id1);

    auto it = std::remove_if(edges.begin(), edges.end(),
        [id1, id2](const std::tuple<int, int, EdgeType>& edge) {
            return (std::get<0>(edge) == id1 && std::get<1>(edge) == id2) ||
                    (std::get<0>(edge) == id2 && std::get<1>(edge) == id1);
        });
    edges.erase(it, edges.end());
}

void ZXGraph::printGraph() const {
    std::cout << "\n-------------------------\n";
    std::cout << "Printing ZX Graph:\n";

    std::cout << "Inputs: ";
    for (int in : getInputs()) {
        std::cout << in << " ";
    }
    std::cout << "\nOutputs: ";
    for (int out : getOutputs()) {
        std::cout << out << " ";
    }
    std::cout << "\n";

    for (const auto& [id, spider] : spiders) {
        std::cout << "Spider " << id
                  << " [" << spiderTypeToString(spider.type)
                  << "], phase: " << spider.phase
                  << ", edges: ";
            for (const auto& [id, type] : spider.neighbors) {
                std::cout << (type == EdgeType::SIMPLE ? "-" : "~") << id << " ";
            }
        std::cout << "\n";
    }

    std::cout << "-------------------------\n";
}

std::vector<int> ZXGraph::getInputs() const {
    std::vector<int> ins;
    for (const auto& [id, spider] : spiders) {
        if (spider.type == SpiderType::INPUT) {
            ins.push_back(id);
        }
    }
    return ins;
}
std::vector<int> ZXGraph::getOutputs() const {
    std::vector<int> outs;
    for (const auto& [id, spider] : spiders) {
        if (spider.type == SpiderType::OUTPUT) {
            outs.push_back(id);
        }
    }
    return outs;
}


ZXGraph ZXGraph::fromQuantumCircuit(const QuantumCircuit& qc) {
    ZXGraph zx;
    std::vector<int> wire_end(qc.num_qubits, -1);

    for (int q = 0; q < qc.num_qubits; ++q)
        wire_end[q] = zx.addSpider(SpiderType::INPUT);

    // Helper: append a single spider on qubit q, return new node id
    auto appendSpider = [&](int q, SpiderType type, double phase = 0.0) {
        int node = zx.addSpider(type, phase);
        zx.addEdge(wire_end[q], node);
        wire_end[q] = node;
        return node;
    };

    // Helper: append a single spider with a Hadamard edge on qubit q
    auto appendSpiderH = [&](int q, SpiderType type, double phase = 0.0) {
        int node = zx.addSpider(type, phase);
        zx.addEdge(wire_end[q], node, EdgeType::HADAMARD);
        wire_end[q] = node;
        return node;
    };

    // Helper: build the shared CCZ interior (given already-placed zc1, zc2, zt nodes)
    auto buildCCZInterior = [&](int zc1, int zc2, int zt) {
        auto addAncilla = [&](double angle, std::initializer_list<int> neighbors) {
            int z     = zx.addSpider(SpiderType::Z);
            int za    = zx.addSpider(SpiderType::Z, angle);
            zx.addEdge(z, za, EdgeType::HADAMARD);
            for (int n : neighbors)
                zx.addEdge(n, z, EdgeType::HADAMARD);
        };
        addAncilla(7 * M_PI / 4, {zc1, zt});
        addAncilla(7 * M_PI / 4, {zc2, zt});
        addAncilla(7 * M_PI / 4, {zc1, zc2});
        addAncilla(    M_PI / 4, {zc1, zc2, zt});
    };

    for (const auto& gate : qc.gates) {
        std::string op = gate.name;
        std::transform(op.begin(), op.end(), op.begin(), ::tolower);

        if      (op == "h")              appendSpiderH(gate.qubits[0], SpiderType::Z);
        else if (op == "x")              appendSpider (gate.qubits[0], SpiderType::X, M_PI);
        else if (op == "z")              appendSpider (gate.qubits[0], SpiderType::Z, M_PI);
        else if (op == "s")              appendSpider (gate.qubits[0], SpiderType::Z,  M_PI/2);
        else if (op == "sdag"||op=="sdg")appendSpider (gate.qubits[0], SpiderType::Z, -M_PI/2);
        else if (op == "t")              appendSpider (gate.qubits[0], SpiderType::Z,  M_PI/4);
        else if (op == "tdag"||op=="tdg")appendSpider (gate.qubits[0], SpiderType::Z, -M_PI/4);
        else if (op == "rz")             appendSpider (gate.qubits[0], SpiderType::Z, gate.params[0]);
        else if (op == "rx")             appendSpider (gate.qubits[0], SpiderType::X, gate.params[0]);

        else if (op == "y" || op == "ry") {
            int q = gate.qubits[0];
            double theta = (op == "ry") ? gate.params[0] : M_PI;
            appendSpider(q, SpiderType::Z,  M_PI/2);
            appendSpider(q, SpiderType::X, theta);
            appendSpider(q, SpiderType::Z, -M_PI/2);

        } else if (op == "cx") {
            int z = zx.addSpider(SpiderType::Z);
            int x = zx.addSpider(SpiderType::X);
            zx.addEdge(z, x);
            zx.addEdge(wire_end[gate.qubits[0]], z); wire_end[gate.qubits[0]] = z;
            zx.addEdge(wire_end[gate.qubits[1]], x); wire_end[gate.qubits[1]] = x;

        } else if (op == "cz") {
            int z1 = zx.addSpider(SpiderType::Z);
            int z2 = zx.addSpider(SpiderType::Z);
            zx.addEdge(z1, z2, EdgeType::HADAMARD);
            zx.addEdge(wire_end[gate.qubits[0]], z1); wire_end[gate.qubits[0]] = z1;
            zx.addEdge(wire_end[gate.qubits[1]], z2); wire_end[gate.qubits[1]] = z2;

        } else if (op == "ccz" || op == "ccx") {
            int c1 = gate.qubits[0], c2 = gate.qubits[1], t = gate.qubits[2];

            int zc1 = appendSpider(c1, SpiderType::Z, M_PI/4);
            int zc2 = appendSpider(c2, SpiderType::Z, M_PI/4);
            int zt  = (op == "ccz")
                ? appendSpider (t, SpiderType::Z, M_PI/4)
                : appendSpiderH(t, SpiderType::Z, M_PI/4);

            buildCCZInterior(zc1, zc2, zt);

            if (op == "ccx") {
                // close the H-sandwich around target
                int h_end = zx.addSpider(SpiderType::Z);
                zx.addEdge(zt, h_end, EdgeType::HADAMARD);
                wire_end[t] = h_end;
            }

        } else if (op == "measure") {
            continue;
        } else {
            std::cerr << "Unsupported gate: " << gate.name << "\n";
        }
    }

    for (int q = 0; q < qc.num_qubits; ++q) {
        int output = zx.addSpider(SpiderType::OUTPUT);
        zx.addEdge(wire_end[q], output);
        wire_end[q] = output;
    }

    return zx;
}


json ZXGraph::toJson() const {
    json j;

    int size = spiders.size();
    j["size"] = size;

    std::vector<json> nodeVec(size, nullptr);
    for (const auto& [id, spider] : spiders) {
        std::string spiderStr = spiderTypeToString(spider.type);
        std::string angleStr = radiansToString(spider.phase);
        nodeVec[id] = { spiderStr, angleStr };
    }
    j["nodes"] = nodeVec;

    json edgeList = json::array();
    for (const auto& [id1, id2, type] : edges) {
        edgeList.push_back({ id1, id2, edgeTypeToInt(type) });
    }
    j["edges"] = edgeList;
    j["inputs"] = getInputs();
    j["outputs"] = getOutputs();

    return j;
}


ZXGraph ZXGraph::clone() const {
    ZXGraph copy;
    copy.next_spider_id = this->next_spider_id;
    copy.spiders = this->spiders;
    copy.edges = this->edges;
    return copy;
}







// ################ Transformation to MBQC: #########################

// to_gh(res, quiet=True) #red to green
void ZXGraph::toGH() {
    for (auto& [id, spider] : spiders) {
        if (spider.type == SpiderType::X) {
            spider.type = SpiderType::Z;

            for (auto& [neighbor_id, etype] : spider.neighbors) {
                EdgeType new_type = (etype == EdgeType::SIMPLE) ? EdgeType::HADAMARD : EdgeType::SIMPLE;
                spider.neighbors[neighbor_id] = new_type;
                spiders[neighbor_id].neighbors[id] = new_type;

                for (auto& edge : edges) {
                    int u = std::get<0>(edge);
                    int v = std::get<1>(edge);
                    if ((u == id && v == neighbor_id) || (u == neighbor_id && v == id)) {
                        std::get<2>(edge) = new_type;
                    }
                }
            }
        }
    }
}


// #####  spider_simp(res, quiet=True) #maximally fuse green spiders
bool canFuse(const Spider& a, const Spider& b, EdgeType edgeType) {
    return a.type == SpiderType::Z && b.type == SpiderType::Z && edgeType == EdgeType::SIMPLE;
}

void ZXGraph::fuseSpiders(int keepId, int removeId) {
    auto& keep = spiders[keepId];
    auto& rem = spiders[removeId];

    // Add phase
    float keepPhase = keep.phase;

    std::vector<std::pair<int, int>> edgesToRemove;

    // Rewire neighbors
    for (const auto& [neighborId, neighborEdgeType] : rem.neighbors) {
        if (neighborId == keepId) continue; // Skip the edge between keep and rem (over which the fuse is happening)

        // Handle duplicate edges
        if (keep.neighbors.find(neighborId) != keep.neighbors.end()) {
            EdgeType duplicateEdgeType = keep.neighbors[neighborId];

            if (duplicateEdgeType == neighborEdgeType) {
                removeEdge(keepId, neighborId);
            } else {
                // Let the edge between keep and neighbor be simple and add a pi to the phase of keep
                removeEdge(keepId, neighborId);
                addEdge(keepId, neighborId, EdgeType::SIMPLE);
                keepPhase += M_PI;
            }

        } else {
            addEdge(keepId, neighborId, neighborEdgeType);
        }
        
        // Deferring this action because of segmentation violation (Changing rem.neighbors while iterating over it)
        edgesToRemove.emplace_back(neighborId, removeId);
    }

    for (const auto& [a, b] : edgesToRemove) {
        removeEdge(a, b);
    }

    keepPhase += rem.phase;
    keep.phase = normalize_radians(keepPhase);

    removeEdge(keepId, removeId);
    spiders.erase(removeId);
}


std::vector<std::pair<int, int>> findFusableGreenSpiders(const ZXGraph& g) {
    std::vector<std::pair<int, int>> pairs;

    for (const auto& [id, spider] : g.spiders) {
        for (const auto& [neighborId, edgeType] : spider.neighbors) {
            if (id < neighborId && canFuse(spider, g.spiders.at(neighborId), edgeType)) {
                pairs.emplace_back(id, neighborId);
            }
        }
    }
    return pairs;
}

void ZXGraph::spiderSimplification() {
    while (true) {
        auto matches = findFusableGreenSpiders(*this);
        if (matches.empty()) break;

        for (auto [a, b] : matches) {
            // Ensure both spiders still exist
            if (spiders.count(a) == 0 || spiders.count(b) == 0) continue;

            // Always keep the lower ID to simplify logic
            if (a > b) std::swap(a, b);
            fuseSpiders(a, b);
        }
    }
}


// ### handle Inputs and Outputs ###


// Takes two node ids and inserts HADAMARD Z HADAMARD between them. Returns the ID of the inserted Z node
int ZXGraph::insert_HZH(int id1, int id2) {
    int new_z = addSpider(SpiderType::Z);
    addEdge(id1, new_z, EdgeType::HADAMARD);
    addEdge(new_z, id2, EdgeType::HADAMARD);
    return new_z;
}


// Make all outgoing edges from inputs to HADAMARD 
void ZXGraph::handleInputs() {
    
    std::vector<int> inputIds;  // Needed because insertHZH alters the spiders map!
    for (const auto& [id, spider] : spiders) {
        if (spider.type == SpiderType::INPUT)
            inputIds.push_back(id);
    }


    for (int inputId : inputIds) {
        const auto& inputSpider = spiders.at(inputId);

        if (inputSpider.neighbors.size() != 1) {
            throw std::runtime_error(
                std::string("Input ")
                + std::to_string(inputId)
                + " is not connected to exactly one spider.\n"
            );
        }
        
        auto [neighborId, edgeType] = *inputSpider.neighbors.begin();

        if (edgeType == EdgeType::HADAMARD) {
            continue;
        }
        
        removeEdge(inputId, neighborId);
        insert_HZH(inputId, neighborId);

    }
}


// Make all outgoing edges from outputs to HADAMARD 
void ZXGraph::handleOutputs() {

    std::vector<int> outputIds;  // Needed because insertHZH alters the spiders map!
    for (const auto& [id, spider] : spiders) {
        if (spider.type == SpiderType::OUTPUT)
            outputIds.push_back(id);
    }

    for (int outputId: outputIds) {
        const auto& outputSpider = spiders.at(outputId);

        if (outputSpider.neighbors.size() != 1) {
            throw std::runtime_error(
                std::string("Output ")
                + std::to_string(outputId)
                + " is not connected to exactly one spider.\n"
            );
        }
        
        auto [neighborId, edgeType] = *outputSpider.neighbors.begin();

        if (edgeType == EdgeType::HADAMARD) {
            continue;
        }

        removeEdge(neighborId, outputId);
        insert_HZH(neighborId, outputId);
    }
}


// TODO: disentangle_outputs(res) #outputs should not be interconnected




// ############## From MBQC and tensor comparison ########################


json ZXGraph::toPyZXJson() const {
    json j;
    
    j["version"] = 2;
    j["backend"] = "simple";  // instead of multigraph

    std::vector<int> inputs = getInputs();
    
    j["inputs"] = inputs;
    j["outputs"] = getOutputs();

    std::vector<json> vertices;
    for (const auto& [id, spider] : spiders) {
        json vertice;
        vertice["id"] = id;
        vertice["phase"] = radiansToString(spider.phase);
        vertice["t"] = spiderTypeToT(spider.type);
        vertice["pos"] = {( std::find(inputs.begin(), inputs.end(), id) != inputs.end() ) ? -1: 0, 0};  // Important that Inputs have lower indices than their neighbors!
        vertices.push_back(vertice);
    }
    j["vertices"] = vertices;

    json edgeList = json::array();
    for (const auto& [id1, id2, type] : edges) {
        edgeList.push_back({ id1, id2, edgeTypeToInt(type) });
    }

    j["edges"] = edgeList;


    return j;
}