#ifndef FLOW_HPP
#define FLOW_HPP


#include "MBQC_Graph.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>
#include <iostream>

struct PauliFlowResult {
    std::unordered_map<int, std::unordered_set<int>> corrf;
    std::unordered_map<int, std::unordered_set<int>> oddNcorrf;
    std::vector<int> depths;
    bool ok = false;
};

PauliFlowResult findPauliFlow(const MBQC_Graph& g);

json PauliFlowResultToJson(const PauliFlowResult& result);

void focus(PauliFlowResult& flow, const MBQC_Graph& g);

#endif
