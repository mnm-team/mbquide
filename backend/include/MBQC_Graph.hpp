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
    const std::vector<std::vector<int>>& getAdjacencyMatrix() const;
    const int getSize() const;
    const std::map<int, OutputAdjustmentMap>& getOutputAdjustments() const;
    OutputAdjustmentMap& getOutputAdjustment(int u);  // needs to be call by ref in order to call: graph.getOutputAdjustment(u).adjustOutput("X"); (for simulator)
    OutputAdjustmentMap getOutputAdjustment(int u) const;

    std::pair<MeasurementBasis, double> getMeasurement(int node) const;
    
    void printGraph() const;

    MBQC_Graph clone() const;

    bool isInput(int u) const;
    bool isOutput(int u) const;

    std::vector<int> getOutputs() const;
    std::vector<int> getInputs() const;

    
    // Operations: 
    void localComplementation(int u);
    void pivot(int u, int v);
    void ZInsertion(const std::vector<int>& inputVertices);
    void ZDeletion(int u);
    void relabel(int u);
    void relabelPlanar(int u, MeasurementBasis preferredBasis);
    void relabelPlanar(int u);

    // FLOW
    std::vector<int> getNonOutputs() const;
    std::vector<int> getNonInputs() const;
    std::vector<std::vector<int>> getFlowDemandMatrix() const;
    std::unordered_set<int> oddNeighborhood(const std::unordered_set<int>& S) const;
    std::vector<int> mvertices() const;

    // JSON: 
    json toJson() const;
    void exportToPYZXJsonFile(const std::string& filename, int rowLength = 4) const;
    static MBQC_Graph fromJson(const json& j);
    static MBQC_Graph importFromPYZXJsonFile(const std::string& filename);

private:
    int size;
    std::vector<std::vector<int>> adjacencyMatrix;
    std::map<int, std::pair<MeasurementBasis, double>> measurements;
    std::vector<int> inputs;
    std::vector<int> outputs;
    std::map<int, OutputAdjustmentMap> outputAdjustments;
};

#endif
