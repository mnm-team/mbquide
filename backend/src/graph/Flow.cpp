#include "Flow.hpp"

// ========== JSON util ==========
json PauliFlowResultToJson(const PauliFlowResult& result) {
    json j;
    
    // Add the ok field
    j["ok"] = result.ok;
    
    // Convert corrf map  (X-corrections)
    json corrf_json = json::object();
    for (const auto& [key, value_set] : result.corrf) {
        // Convert the set to a JSON array
        json set_array = json::array();
        for (const int value : value_set) {
            set_array.push_back(value);
        }
        // Use string key for JSON compatibility
        corrf_json[std::to_string(key)] = set_array;
    }
    j["corrf"] = corrf_json;

    
    // Convert oddNcorrf map  (Z-corrections)
    json oddNcorrf_json = json::object();
    for (const auto& [key, value_set] : result.oddNcorrf) {
        // Convert the set to a JSON array
        json set_array = json::array();
        for (const int value : value_set) {
            set_array.push_back(value);
        }
        // Use string key for JSON compatibility
        oddNcorrf_json[std::to_string(key)] = set_array;
    }
    j["oddNcorrf"] = oddNcorrf_json;
    
    j["depths"] = result.depths;
    
    return j;
}



// ========== buildForcedEdges ==========
// Mainly following https://gitlab.lrz.de/ru46yed/pauli-string-extraction/-/blob/main/src/flow.py?ref_type=heads#L444
// Returns false if no Pauli flow exists (due to P4–P6 or cycle)
bool buildForcedEdges(
    const MBQC_Graph& g,
    const std::unordered_map<int, std::unordered_set<int>>& corrf,
    std::vector<std::pair<int,int>>& forcedEdges)
{
    forcedEdges.clear();
    std::unordered_map<int, std::unordered_set<int>> Odd_corrf;

    auto nonOut = g.getNonOutputs();

    for (int u : nonOut) {
        const auto correctionPair = corrf.find(u);  // Finds the pait in corrf (u |-> {non-inputs})
        const std::unordered_set<int>& uCorrectionSet = (correctionPair == corrf.end()) ? std::unordered_set<int>() : correctionPair->second; //creates empty set if no u was found, else the set is the set of u in the corrf

        Odd_corrf[u] = g.oddNeighborhood(uCorrectionSet);

        auto [basis_u, _angle] = g.getMeasurement(u);

        // TODO: How does this match with P4a, P4b .... in arXiv:2410.23439
        if (basis_u == MeasurementBasis::XY && uCorrectionSet.count(u)) return false;  // P4
        if (basis_u == MeasurementBasis::XZ && !Odd_corrf[u].count(u)) return false;  // P5
        if (basis_u == MeasurementBasis::YZ && Odd_corrf[u].count(u)) return false;  // P6 

        // P1
        for (int v : uCorrectionSet) {
            if (u != v) {
                auto [bv, _] = g.getMeasurement(v);
                if (bv != MeasurementBasis::X && bv != MeasurementBasis::Y) forcedEdges.emplace_back(u, v);
            }
        }
        // P2
        for (int v : Odd_corrf[u]) {
            if (u != v) {
                auto [bv, _] = g.getMeasurement(v);
                if (bv != MeasurementBasis::Y && bv != MeasurementBasis::Z) forcedEdges.emplace_back(u, v);
            }
        }
        // P3 ??
        for (int v : nonOut) {
            if (u == v) continue;
            auto [bv, _] = g.getMeasurement(v);
            if (bv == MeasurementBasis::Y) {
                bool inCorr = uCorrectionSet.count(v) > 0;
                bool inOdd  = Odd_corrf[u].count(v) > 0;
                if (inCorr ^ inOdd) forcedEdges.emplace_back(u, v);
            }
        }
    }

    return computeTransitiveClosure(nonOut, forcedEdges);
}

// ========== reconstructOrder ==========
// Mainly following https://gitlab.lrz.de/ru46yed/pauli-string-extraction/-/blob/main/src/flow.py?ref_type=heads#L477
std::vector<int> reconstructOrder(
    const MBQC_Graph& g,
    const std::vector<std::pair<int,int>>& forcedEdges)
{
    
    std::unordered_map<int, std::vector<int>> after;
    for (auto [u,v] : forcedEdges) after[u].push_back(v);

    std::unordered_set<int> processed;
    std::unordered_map<int,int> res;
    for (int o : g.getOutputs()) { // outputs start at depth 0
        processed.insert(o);
        res[o] = 0;
    }

    
    std::unordered_set<int> allMeasured;
    for (int v : g.mvertices()) {
        allMeasured.insert(v);
    }
    
    int d = 1;
    while ((int)processed.size() < g.getSize()) {
        std::vector<int> newly;
        for (int v : allMeasured) {
            if (processed.count(v)) continue;
            auto it = after.find(v);
            if (it == after.end()) {
                newly.push_back(v);
                continue;
            }
            bool ok = true;
            for (int w : it->second) {
                if (!processed.count(w)) { ok = false; break; }
            }
            if (ok) newly.push_back(v);
        }
        if (newly.empty()) break;
        for (int v : newly) { processed.insert(v); res[v] = d; }
        d++;
    }

    // Change Map to list of integers, because map has sequential integers starting from 0 (the vertice id)
    std::vector<int> resultVec(g.getSize());
    for (const auto& [index, depth] : res) {
        resultVec[index] = depth;
    }

    return resultVec;
}


// ========== findPauliFlow ==========
PauliFlowResult findPauliFlow(const MBQC_Graph& g) {
    PauliFlowResult out;

    auto M = g.getFlowDemandMatrix();
    auto N = getInverse(M);
    if (N.empty()) {
        // No flow can be found
        return out;
    }

    auto nonOut = g.getNonOutputs();
    auto nonIn  = g.getNonInputs();

    // Build corrf[v] from N
    for (int i = 0; i < (int)nonOut.size(); ++i) {
        int v = nonOut[i];
        std::unordered_set<int> S;
        for (int j = 0; j < (int)nonIn.size(); ++j) {
            if (abs(N[j][i]) == 1) S.insert(nonIn[j]);  // abs because we calculate the inverse in R but we need GF(2)
        }
        out.corrf[v] = std::move(S);

        // Handle oddNeighbors
        std::unordered_set<int> oddN = g.oddNeighborhood(out.corrf[v]);
        oddN.erase(v);
        out.oddNcorrf[v] = std::move(oddN);
    }

    std::vector<std::pair<int,int>> forcedEdges;
    if (!buildForcedEdges(g, out.corrf, forcedEdges)) {
        // No flow can be found (violations or cycles)
        return out;
    }

    out.depths = reconstructOrder(g, forcedEdges);
    out.ok = true;
    return out;
}


// ========== Focus ==========
// Mainly following https://gitlab.lrz.de/ru46yed/pauli-string-extraction/-/blob/main/src/flow.py?ref_type=heads#L12
// Definition 4.3 in http://arxiv.org/abs/2109.05654 (Relating Measurement Patterns to Circuits via Pauli Flow)
// Definition 2.5 in http://arxiv.org/abs/2410.23439 (An algebraic interpretation of Pauli flow)

// PauliFlowResult focus(const PauliFlowResult& p, const MBQC_Graph g) {}
void focus(PauliFlowResult& flow, const MBQC_Graph& g) {

    if (!flow.ok) {
        std::cout << "Cannot focus this flow because no flow was found!";
    }

    // Create order: map < depth -> list of vertices >
    std::map<int, std::vector<int>> order;
    for (size_t i = 0; i < flow.depths.size(); ++i) {
        int depth = flow.depths[i];
        int vertex = static_cast<int>(i);
        
        if (order.find(depth) != order.end()) {
            order[depth].push_back(vertex);
        } else {
            order[depth] = {vertex};
        }
    }
    
    // Convert inputs and outputs to sets for faster lookup
    auto inputs_vec = g.getInputs();
    std::unordered_set<int> inputs_set(inputs_vec.begin(), inputs_vec.end());
    auto outputs_vec = g.getOutputs();
    std::unordered_set<int> outputs_set(outputs_vec.begin(), outputs_vec.end());
    

    for (const auto& [depth, vertices] : order) {
        if (depth == 0) {
            // Skip outputs as they do not have corrections
            continue;
        }
        
        for (int v : vertices) {
            if (flow.corrf.find(v) == flow.corrf.end()) {
                continue; // Skip if vertex not in corrections
            }
            
            const auto& corrections = flow.corrf[v];
            
            // Get odd neighborhood excluding inputs
            auto odd_n = g.oddNeighborhood(corrections);
            for (auto n = odd_n.begin(); n != odd_n.end();) {
                if (inputs_set.find(*n) != inputs_set.end()) {
                    n = odd_n.erase(n);
                } else {
                    ++n;
                }
            }
            
            // Initialize parities
            std::unordered_map<int, int> parities;
            for (int correction : corrections) {
                parities[correction] = 1;
            }
            
            
            for (int correction : corrections) {

                auto [basis_corr, _angle] = g.getMeasurement(correction);
                

                if (correction == v || 
                    basis_corr == MeasurementBasis::XY ||
                    basis_corr == MeasurementBasis::X) {
                    continue;
                }
                
                if (basis_corr == MeasurementBasis::Y && 
                    odd_n.find(correction) != odd_n.end()) {
                    continue;
                }
                
                if (flow.corrf.find(correction) != flow.corrf.end()) {
                    for (int w : flow.corrf[correction]) {
                        if (parities.find(w) != parities.end()) {
                            parities[w] += 1;
                        } else {
                            parities[w] = 1;
                        }
                    }
                }
            }
            
            for (int w : odd_n) {

                auto [basis_w, _angle] = g.getMeasurement(w);
                
                if (v == w || 
                    outputs_set.find(w) != outputs_set.end() ||
                    basis_w == MeasurementBasis::XZ ||
                    basis_w == MeasurementBasis::YZ ||
                    basis_w == MeasurementBasis::Z) {
                    continue;
                }
                
                if (basis_w == MeasurementBasis::Y && 
                    corrections.find(w) != corrections.end()) {
                    continue;
                }
                
                if (flow.corrf.find(w) != flow.corrf.end()) {
                    for (int correction : flow.corrf[w]) {
                        if (parities.find(correction) != parities.end()) {
                            parities[correction] += 1;
                        } else {
                            parities[correction] = 1;
                        }
                    }
                }
            }
            
            // Build new correction set from odd parities
            std::unordered_set<int> new_c;
            for (const auto& [w, parity] : parities) {
                if (parity % 2 == 1) {
                    new_c.insert(w);
                }
            }
            
            flow.corrf[v] = std::move(new_c);

            // Handle oddNeighbors
            std::unordered_set<int> oddN = g.oddNeighborhood(flow.corrf[v]);
            oddN.erase(v);
            flow.oddNcorrf[v] = std::move(oddN);
        }
    }
}
