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


// ========== Build Matrices ==========

// Build the flow-demand matrix M  (Definition 3.4, arXiv:2410.23439)
//
// Rows  ↔ nonOutputs  (size n − nO)
// Cols  ↔ nonInputs   (size n − nI)
//
// Rules per measurement label λ(v) of a non-output vertex v:
//   {X, XY}     → row = adjacency of v restricted to non-inputs
//   {Z, YZ, XZ} → M[v,v] = 1  (only if v ∉ I)
//   Y           → adjacency of v restricted to non-inputs + F[v,v] = 1
//
static GF2Mat buildFlowDemandMatrix(const MBQC_Graph& g) {

    auto nonOutputs = g.getNonOutputs();
    auto nonInputs  = g.getNonInputs();

    int nRows = (int)nonOutputs.size();
    int nCols = (int)nonInputs.size();

    const auto& adj = g.getAdjacencyMatrix();

    // Map non-input vertex id -> column index
    std::unordered_map<int,int> colIdx;
    for (int j = 0; j < nCols; ++j) {
        colIdx[nonInputs[j]] = j;
    }

    GF2Mat M(nRows, std::vector<int>(nCols, 0));

    for (int i = 0; i < nRows; ++i) {

        int v = nonOutputs[i];
        auto [basis, _angle] = g.getMeasurement(v);

        switch (basis) {

            // Case 1: {X, XY}
            case MeasurementBasis::X:
            case MeasurementBasis::XY:

                // neighbourhood of v among non-inputs
                for (int j = 0; j < nCols; ++j) {
                    M[i][j] = adj[v][nonInputs[j]];
                }
                break;

            // Case 2: {Z, YZ, XZ}
            case MeasurementBasis::Z:
            case MeasurementBasis::YZ:
            case MeasurementBasis::XZ:

                // only diagonal entry if v is a non-input
                if (colIdx.count(v)) {
                    M[i][colIdx.at(v)] = 1;
                }
                break;

            // Case 3: {Y}
            case MeasurementBasis::Y:

                // neighbourhood + self-loop
                for (int j = 0; j < nCols; ++j) {
                    M[i][j] = adj[v][nonInputs[j]];
                }

                if (colIdx.count(v)) {
                    M[i][colIdx.at(v)] = 1;
                }
                break;

            default:
                break;
        }
    }

    return M;
}

//  Build the order-demand matrix N  (Definition 3.5, arXiv:2410.23439)
//
//  Rows  ↔ nonOutputs  (size n − nO)
//  Cols  ↔ nonInputs   (size n − nI)
//
//  Rules per measurement label λ(v) of a non-output vertex v:
//    {X, Y, Z}  → row is all-zero
//    XY         → N[v,v] = 1  (only, if v ∉ I)
//    YZ         → row = adjacency of v restricted to non-inputs (no self-loop)
//    XZ         → adjacency of v restricted to non-inputs + N[v,v] = 1  (if v ∉ I)
static GF2Mat buildOrderDemandMatrix(const MBQC_Graph& g) {
    auto nonOutputs = g.getNonOutputs();
    auto nonInputs  = g.getNonInputs();
    int nRows = (int)nonOutputs.size();
    int nCols = (int)nonInputs.size();
    const auto& adj = g.getAdjacencyMatrix();
 
    // Map non-input vertex id → column index
    std::unordered_map<int,int> colIdx;
    for (int j = 0; j < nCols; ++j) colIdx[nonInputs[j]] = j;
 
    GF2Mat N(nRows, std::vector<int>(nCols, 0));
 
    for (int i = 0; i < nRows; ++i) {
        int v = nonOutputs[i];
        auto [basis, _angle] = g.getMeasurement(v);
 
        switch (basis) {
            case MeasurementBasis::X:
            case MeasurementBasis::Y:
            case MeasurementBasis::Z:
                // row stays all-zero
                break;
 
            case MeasurementBasis::XY:
                // only the (v,v) diagonal entry, provided v is a non-input
                if (colIdx.count(v))
                    N[i][colIdx.at(v)] = 1;
                break;
 
            case MeasurementBasis::YZ:
                // neighbourhood of v among non-inputs, no self-loop
                for (int j = 0; j < nCols; ++j) {
                    int w = nonInputs[j];
                    if (w != v) N[i][j] = adj[v][w];
                }
                break;
 
            case MeasurementBasis::XZ:
                // neighbourhood of v among non-inputs + self-loop entry
                for (int j = 0; j < nCols; ++j)
                    N[i][j] = adj[v][nonInputs[j]];
                if (colIdx.count(v))
                    N[i][colIdx.at(v)] = 1;
                break;
 
            default:
                break;
        }
    }
    return N;
}



//  Reconstruct per-vertex depths from the NC adjacency matrix.
//
//  NC[w][v] == 1  ⟺  v ⊳_c w  (v must be measured strictly before w).
//  Outputs are assigned depth 0; all other vertices get increasing depths.
static std::vector<int> depthsFromNC(
    const GF2Mat& NC,
    const std::vector<int>& nonOutputs,
    const std::vector<int>& outputs,
    int graphSize)
{
    int nO_bar = (int)nonOutputs.size();
 
    // after[v] = set of vertices w such that v must precede w
    std::unordered_map<int,std::vector<int>> after;
    for (int wi = 0; wi < nO_bar; ++wi)
        for (int vi = 0; vi < nO_bar; ++vi)
            if (NC[wi][vi])
                after[nonOutputs[vi]].push_back(nonOutputs[wi]);
 
    std::unordered_map<int,int> res;
    std::unordered_set<int> processed;
    for (int o : outputs) { res[o] = 0; processed.insert(o); }
 
    int d = 1;
    while ((int)processed.size() < graphSize) {
        std::vector<int> newly;
        for (int v : nonOutputs) {
            if (processed.count(v)) continue;
            auto it = after.find(v);
            bool ready = (it == after.end());
            if (!ready) {
                ready = true;
                for (int w : it->second)
                    if (!processed.count(w)) { ready = false; break; }
            }
            if (ready) newly.push_back(v);
        }
        if (newly.empty()) break;
        for (int v : newly) { processed.insert(v); res[v] = d; }
        ++d;
    }
 
    std::vector<int> depths(graphSize, 0);
    for (auto& [v, dep] : res) depths[v] = dep;
    return depths;
}


// ========== findPauliFlow ==========

//  findPauliFlow  —  O(n³) algorithm, Theorem 4.4 in Mitosek & Backens, arXiv:2410.23439
//
//  Works for any nI ≤ nO.  
// When nI == nO it's the same as the simple inversion approach (kerDim == 0, no layer-by-layer loop needed).
PauliFlowResult findPauliFlow(const MBQC_Graph& g) {
    PauliFlowResult out;
 
    auto nonOutputs = g.getNonOutputs();  // O-bar,  size = n − nO
    auto nonInputs  = g.getNonInputs();   // I-bar,  size = n − nI
    auto outputs    = g.getOutputs();
    auto inputs     = g.getInputs();
 
    int nO_bar = (int)nonOutputs.size();
    int nI_bar = (int)nonInputs.size();
    int kerDim = (int)outputs.size() - (int)inputs.size();  // nO − nI
 
    // nO < nI  →  M has more rows than columns, never right-invertible
    if (kerDim < 0) return out;
 
    // ── Step 1 ────────────────────────────────────────────────────────────────
    // Flow-demand matrix M  (Definition 3.4),  size (n-nO) × (n-nI)
    GF2Mat M = buildFlowDemandMatrix(g);
 
    // ── Step 2 ────────────────────────────────────────────────────────────────
    // Order-demand matrix N  (Definition 3.5),  size (n-nO) × (n-nI)
    GF2Mat N = buildOrderDemandMatrix(g);
 
    // ── Steps 3 & 4 ───────────────────────────────────────────────────────────
    // Check right-invertibility and find C0  (right inverse of M).
    // ── Step 5 ────────────────────────────────────────────────────────────────
    // Find F whose columns form a basis of ker M.
    GF2Mat C0, F;
    if (!gf2RightInvAndKernel(M, C0, F)) return out;  // rank M < n-nO → no flow
    // C0 : (n-nI) × (n-nO),   F : (n-nI) × (nO-nI)
 
    // ── Step 6 ────────────────────────────────────────────────────────────────
    // C' = [ C0 | F ],   size (n-nI) × (n-nI)
    GF2Mat Cprime = gf2Hcat(C0, F);
 
    // ── Step 7 ────────────────────────────────────────────────────────────────
    // NB = N * C',   size (n-nO) × (n-nI)
    GF2Mat NB = gf2Mul(N, Cprime);
 
    // ── Step 8 ────────────────────────────────────────────────────────────────
    // NL = first (n-nO) columns of NB,   NR = last (nO-nI) columns of NB
    GF2Mat NL = gf2ColSlice(NB, 0,       nO_bar);
    GF2Mat NR = gf2ColSlice(NB, nO_bar,  nO_bar + kerDim);
 
    // ── Special case: nI == nO  ───────────────────────────────────────────────
    // kerDim == 0,  CB = Id,  NC = NL.  Just check DAG directly.
    if (kerDim == 0) {
        if (!gf2IsDAG(NL)) return out;
        for (int vi = 0; vi < nO_bar; ++vi) {
            int v = nonOutputs[vi];
            std::unordered_set<int> S;
            for (int ji = 0; ji < nI_bar; ++ji)
                if (C0[ji][vi]) S.insert(nonInputs[ji]);
            out.corrf[v] = S;
            std::unordered_set<int> oddN = g.oddNeighborhood(S);
            oddN.erase(v);
            out.oddNcorrf[v] = std::move(oddN);
        }
        out.depths = depthsFromNC(NL, nonOutputs, outputs, g.getSize());
        out.ok = true;
        return out;
    }
 
    // ── Step 9 ────────────────────────────────────────────────────────────────
    // KLS = KILS = [ NR | NL | Id_{O-bar} ]
    //
    // Column layout (nO_bar rows total):
    //   [0 .. kerDim)                      ← coefficient block  (NR)
    //   [kerDim .. kerDim+nO_bar)          ← rhs block          (NL)
    //   [kerDim+nO_bar .. kerDim+2*nO_bar) ← tracker block      (Id)
    GF2Mat KLS  = gf2Hcat(gf2Hcat(NR, NL), gf2Eye(nO_bar));
    GF2Mat KILS = KLS;   // independent copy; NEVER modified after this point
 
    // ── Step 10 ───────────────────────────────────────────────────────────────
    // RREF on KLS, restricted to the coefficient block (first kerDim columns)
    {
        std::vector<int> pC, pR;
        gf2RREF(KLS, kerDim, pC, pR);
    }
 
    // ── Step 11 ───────────────────────────────────────────────────────────────
    // S = ∅,   P = zero matrix of size (kerDim × nO_bar)
    std::unordered_set<int> S;
    GF2Mat P = gf2Zeros(kerDim, nO_bar);
 
    // ── Step 12 ───────────────────────────────────────────────────────────────
    // Layer-by-layer while loop: keep going until every non-output is solved.
    while ((int)S.size() < nO_bar) {
 
        // ── 12a ───────────────────────────────────────────────────────────────
        // rz = index of the first row whose coefficient block is all-zero.
        // A vertex vi is solvable iff its rhs column is all-zero from rz on.
        int rz = nO_bar;
        for (int r = 0; r < nO_bar; ++r) {
            bool zero = true;
            for (int c = 0; c < kerDim; ++c)
                if (KLS[r][c]) { zero = false; break; }
            if (zero) { rz = r; break; }
        }
 
        std::vector<int> L;
        for (int vi = 0; vi < nO_bar; ++vi) {
            if (S.count(vi)) continue;
            int rhsCol = kerDim + vi;
            bool solvable = true;
            for (int r = rz; r < nO_bar; ++r)
                if (KLS[r][rhsCol]) { solvable = false; break; }
            if (solvable) L.push_back(vi);
        }
 
        if (L.empty()) return out;   // ← no Pauli flow exists
 
        // ── 12b & 12c ─────────────────────────────────────────────────────────
        // Read off the solution for each solvable vertex from the RREF.
        // Because we use full (reduced) row echelon form, each pivot row r
        // directly gives:  x[pivotCol] = KLS[r][rhsCol].
        // (Two separate loops: solve first, then remove — see paper note
        //  after Theorem 4.4 re: ordering within a layer.)
        for (int vi : L) {
            int rhsCol = kerDim + vi;
            for (int r = 0; r < nO_bar; ++r) {
                // Find the leading 1 in the coefficient block of row r
                int lc = -1;
                for (int c = 0; c < kerDim; ++c)
                    if (KLS[r][c]) { lc = c; break; }
                if (lc < 0) break;   // reached all-zero coefficient rows
                P[lc][vi] = KLS[r][rhsCol];
            }
        }
 
        // ── 12d ───────────────────────────────────────────────────────────────
        // Remove each solved vertex's row from the system.
        for (int vi : L) {
 
            // ── 12d-i ─────────────────────────────────────────────────────────
            S.insert(vi);
 
            // ── 12d-ii ────────────────────────────────────────────────────────
            // Rows that still carry a dependency on the original v-row,
            // identified by having a 1 in tracker column (kerDim+nO_bar+vi).
            int trackCol = kerDim + nO_bar + vi;
            std::vector<int> R;
            for (int r = 0; r < nO_bar; ++r)
                if (KLS[r][trackCol]) R.push_back(r);
 
            if (R.empty()) continue;
            int rLast = R.back();
 
            // ── 12d-iii ───────────────────────────────────────────────────────
            // XOR rLast into every r in R except rLast, removing their
            // dependency on the original v-row (since rLast carries it).
            for (int idx = 0; idx + 1 < (int)R.size(); ++idx)
                gf2XorRowInto(KLS, R[idx], KLS[rLast]);
 
            // ── 12d-iv ────────────────────────────────────────────────────────
            // XOR the v-row from KILS (the unmodified original system) into
            // rLast to eliminate rLast's own dependency on the v-row.
            gf2XorRowInto(KLS, rLast, KILS[vi]);
 
            // ── 12d-v ─────────────────────────────────────────────────────────
            // Re-reduce rLast against the current pivot rows to restore row
            // echelon form in the coefficient block.
            // We iterate through rows from top; once we hit an all-zero
            // coefficient row we can stop.
            for (int r = 0; r < nO_bar; ++r) {
                if (r == rLast) continue;
                int lc = -1;
                for (int c = 0; c < kerDim; ++c)
                    if (KLS[r][c]) { lc = c; break; }
                if (lc < 0) break;   // hit all-zero coefficient rows
                if (KLS[rLast][lc])
                    gf2XorRowInto(KLS, rLast, KLS[r]);
            }
 
            // ── 12d-vi ────────────────────────────────────────────────────────
            // Swap rLast into its correct echelon position.
            int lcLast = -1;
            for (int c = 0; c < kerDim; ++c)
                if (KLS[rLast][c]) { lcLast = c; break; }
 
            if (lcLast < 0) {
                // rLast is all-zero in the coefficient block.
                // Move it to the first all-zero-coefficient row (rz position).
                int newRz = nO_bar;
                for (int r = 0; r < nO_bar; ++r) {
                    bool z = true;
                    for (int c = 0; c < kerDim; ++c)
                        if (KLS[r][c]) { z = false; break; }
                    if (z) { newRz = r; break; }
                }
                if (newRz < rLast)
                    std::swap(KLS[rLast], KLS[newRz]);
            } else {
                // Bubble rLast upward past rows whose leading column is
                // larger (i.e., comes later), to restore echelon order.
                for (int r = rLast - 1; r >= 0; --r) {
                    int lc = -1;
                    for (int c = 0; c < kerDim; ++c)
                        if (KLS[r][c]) { lc = c; break; }
                    if (lc < 0 || lc > lcLast)
                        std::swap(KLS[r], KLS[r + 1]);
                    else
                        break;
                }
            }
        } // end for v ∈ L  (inner for-loop of Step 12d)
    } // end while  (Step 12)
 
    // ── Step 13 ───────────────────────────────────────────────────────────────
    // CB = [ Id_{O-bar} ; P ],   size (n-nI) × (n-nO)
    GF2Mat CB = gf2Vcat(gf2Eye(nO_bar), P);
 
    // ── Step 14 ───────────────────────────────────────────────────────────────
    // C  = C' * CB      correction matrix,       (n-nI) × (n-nO)
    // NC = NB * CB      order-relation matrix,   (n-nO) × (n-nO)
    GF2Mat C  = gf2Mul(Cprime, CB);
    GF2Mat NC = gf2Mul(NB,     CB);
 
    if (!gf2IsDAG(NC)) return out;   // sanity check
 
    // ── Build PauliFlowResult ─────────────────────────────────────────────────
    for (int vi = 0; vi < nO_bar; ++vi) {
        int v = nonOutputs[vi];
        std::unordered_set<int> corrSet;
        for (int ji = 0; ji < nI_bar; ++ji)
            if (C[ji][vi]) corrSet.insert(nonInputs[ji]);
        out.corrf[v] = corrSet;
 
        std::unordered_set<int> oddN = g.oddNeighborhood(corrSet);
        oddN.erase(v);
        out.oddNcorrf[v] = std::move(oddN);
    }
 
    out.depths = depthsFromNC(NC, nonOutputs, outputs, g.getSize());
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
