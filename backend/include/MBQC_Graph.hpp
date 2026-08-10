#ifndef MBQC_GRAPH_HPP
#define MBQC_GRAPH_HPP

#include "utils.hpp"
#include "OutputAdjustments.hpp"

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;


enum class GraphRewriteRuleType {
    LocalComplementation,
    Pivot
};

// One applied step of greedyOptimizeEdges(): which rule was used, on which vertex/vertices
// (v is unused, i.e. -1, for LocalComplementation), and the edge-count score it achieved.
struct GraphRewriteStep {
    GraphRewriteRuleType rule;
    int u;
    int v;
    int score;
};


class MBQC_Graph {
public:
    // Default constructor to use map<any, MBQC_Graph>
    MBQC_Graph() : MBQC_Graph(0, {}, {}) {}

    MBQC_Graph(int numNodes, const std::vector<int>& inputVertices, const std::vector<int>& outputVertices);

    void addEdge(int u, int v);

    void setMeasurement(int node, MeasurementBasis basis, double angle = 0);
    void setOutputAdjustment(int node, OutputAdjustmentMap oam);

    std::vector<std::pair<int, int>> getAllEdges() const;
    std::vector<int> getNeighbors(int u) const;
    std::unordered_set<int> oddNeighborhood(const std::unordered_set<int>& S) const;
    const std::vector<std::vector<int>>& getAdjacencyMatrix() const;
    const int getSize() const;
    const std::map<int, OutputAdjustmentMap>& getOutputAdjustments() const;
    OutputAdjustmentMap& getOutputAdjustment(int u);  // needs to be call by ref in order to call: graph.getOutputAdjustment(u).adjustOutput("X"); (for simulator)
    OutputAdjustmentMap getOutputAdjustment(int u) const;

    std::pair<MeasurementBasis, double> getMeasurement(int node) const;
    
    void printGraph() const;
    std::string stateHash() const;
    
    MBQC_Graph clone() const;
    
    bool isInput(int u) const;
    bool isOutput(int u) const;
    
    std::vector<int> getOutputs() const;
    std::vector<int> getInputs() const;
    std::vector<int> getNonOutputs() const;
    std::vector<int> getNonInputs() const;
    std::vector<int> mvertices() const;
    
    
    // Operations: 
    void localComplementation(int u);
    void pivot(int u, int v);
    void ZInsertion(const std::vector<int>& inputVertices);
    void ZDeletion(int u);
    void ZDeletion(std::vector<int> nodes);
    void relabel(int u);
    void relabelPlanar(int u, MeasurementBasis preferredBasis);
    void relabelPlanar(int u);
    void mergeYZ(int u, int v);
    
    // Simplification:
    void simplify(int maxIterations = 1000);
    bool mergeAllYZNodes();
    std::vector<GraphRewriteStep> greedyOptimizeEdges(bool favorVertexRemoval = true);

    // Whether greedyOptimizeEdges() would apply at least one rewrite if called right now.
    // Runs the real algorithm on a throwaway clone, so it stays exactly in sync with
    // greedyOptimizeEdges() instead of duplicating its eligibility logic. Intended for UI
    // enablement checks where actually mutating the graph isn't wanted.
    bool canOptimizeEdges(bool favorVertexRemoval = true) const;

    // JSON:
    json toJson() const;
    void exportToPYZXJsonFile(const std::string& filename, int rowLength = 4) const;
    static MBQC_Graph fromJson(const json& j);
    static MBQC_Graph importFromPYZXJsonFile(const std::string& filename);
    
private:
    // Cost (in saved edges) of a local complementation on v, restricted to a candidate
    // neighbor subset (the "nu-set"). neighborSubset == getNeighbors(v) is the ordinary,
    // full-neighborhood local complementation; any other subset implies vertex "unfusion".
    int lcompCost(int v, const std::vector<int>& neighborSubset) const;

    // Cost (in saved edges) of a pivot on edge (u,v), restricted to candidate neighbor
    // subsets of u and v. Mirrors lcompCost's nu-set generalization for pivot.
    int pivotCost(int u, int v, const std::vector<int>& neighborsU, const std::vector<int>& neighborsV) const;

    // Greedily grows a nu-set (partial neighborhood) for a local complementation on v that
    // locally maximizes lcompCost. Returns the chosen subset and its score.
    std::pair<std::vector<int>, int> findBestLcompNuSet(int v) const;

    // Greedily grows nu-sets (partial neighborhoods) for a pivot on (u,v) that locally
    // maximizes pivotCost. Returns the chosen (subsetU, subsetV) pair and its score.
    std::pair<std::pair<std::vector<int>, std::vector<int>>, int> findBestPivotNuSets(int u, int v) const;

    // Whether applying `rule` on this vertex/edge would newly expose a Pauli node for
    // ZDeletion, used as a tie-break to still apply zero-score rewrites.
    bool ruleFavorsZDeletion(bool isPivot, int u, int v) const;

    // Local complementation restricted to neighborSubset. If neighborSubset is exactly u's
    // current full neighborhood, this is an ordinary localComplementation(u). Otherwise, u is
    // "unfused" first: a fresh Z(0) helper vertex is inserted, connected to neighborSubset and
    // u, and the local complementation is applied to that helper vertex instead. Returns the
    // vertex the complementation was actually applied to (u, or the new helper vertex).
    int lcompRewrite(int u, const std::vector<int>& neighborSubset);

    // Pivot on (u,v) restricted to candidate neighbor subsets of u and v (via three
    // lcompRewrite calls), unfusing u and/or v as needed. Returns the (possibly new) vertex
    // identities that ended up playing the roles of u and v after the rewrite.
    std::pair<int, int> pivotRewrite(int u, int v, const std::vector<int>& neighborsU, const std::vector<int>& neighborsV);

    int size;
    std::vector<std::vector<int>> adjacencyMatrix;
    std::map<int, std::pair<MeasurementBasis, double>> measurements;
    std::vector<int> inputs;
    std::vector<int> outputs;
    std::map<int, OutputAdjustmentMap> outputAdjustments;
};

#endif
