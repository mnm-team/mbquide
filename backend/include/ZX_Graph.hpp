#ifndef ZXGRAPH_HPP
#define ZXGRAPH_HPP

#include "Quantum_Circuit.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>

#include <json.hpp>

using json = nlohmann::json;

enum class SpiderType { Z, X, INPUT, OUTPUT };
enum class EdgeType { SIMPLE, HADAMARD };

std::string spiderTypeToString(SpiderType s);

struct Spider {
    int id;
    SpiderType type;
    double phase;
    std::unordered_map<int, EdgeType> neighbors; // Map of connected spiders
};

class ZXGraph {
private:
    int next_spider_id = 0;

public:
    std::unordered_map<int, Spider> spiders;
    using Edge = std::tuple<int, int, EdgeType>;
    std::vector<Edge> edges;

    int addSpider(SpiderType type, double phase = 0.0);
    void addEdge(int id1, int id2, EdgeType type = EdgeType::SIMPLE);
    void removeEdge(int id1, int id2);
    void printGraph() const;
    static ZXGraph fromQuantumCircuit(const QuantumCircuit& qc);
    std::vector<int> getInputs() const;
    std::vector<int> getOutputs() const;
    json toJson() const;
    ZXGraph clone() const;
    int insert_HZH(int id1, int id2); // Takes two node ids and inserts HADAMARD Z HADAMARD between them. Returns the ID of the inserted Z node

    // Transformation to MBQC:
    void toGH();
    void fuseSpiders(int keepId, int removeId);
    void spiderSimplification();
    void handleInputs();
    void handleOutputs();

    // Compare tensors:
    json toPyZXJson() const;

};

#endif
