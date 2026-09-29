#ifndef FLOW_HPP
#define FLOW_HPP


#include "MBQC_Graph.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>
#include <iostream>

/**
 * @brief The result of findPauliFlow(): a Pauli flow for an MBQC_Graph, i.e. a
 * proof that the pattern can be run with deterministic corrections.
 *
 * A Pauli flow assigns each non-output vertex `v` a *correction set*
 * (`corrf[v]`): the set of input-side vertices whose Pauli corrections must be
 * flipped when `v`'s measurement gives the "wrong" outcome, together with a
 * partial order (`depths`) on measurements that makes those corrections
 * consistent. See findPauliFlow() and focus() for how `corrf`/`oddNcorrf` are
 * produced and used.
 */
struct PauliFlowResult {
    /// For each non-output vertex `v`, the set of vertices to X-correct if `v` measures "1". Keyed by `v`.
    std::unordered_map<int, std::unordered_set<int>> corrf;
    /// For each non-output vertex `v`, the odd neighborhood of `corrf[v]` (minus `v` itself) — the set of vertices to Z-correct if `v` measures "1". Keyed by `v`.
    std::unordered_map<int, std::unordered_set<int>> oddNcorrf;
    /// Per-vertex measurement depth/layer (outputs are depth 0; a vertex's depth is always greater than every vertex that must be measured after it, per the flow's partial order). Indexed by vertex id.
    std::vector<int> depths;
    /// Whether a valid Pauli flow was found. When `false`, `corrf`/`oddNcorrf`/`depths` are empty/unspecified.
    bool ok = false;
};

/**
 * @brief Finds a Pauli flow for `g`, if one exists.
 *
 * Implements the O(n³) algorithm of Theorem 4.4 in Mitosek & Backens,
 * "An algebraic interpretation of Pauli flow" (<https://arxiv.org/abs/2410.23439>), which works
 * for any graph with `#inputs <= #outputs`.
 *
 * @param g The MBQC graph to find a flow for.
 * @return A PauliFlowResult with `ok == true` and populated `corrf`/
 * `oddNcorrf`/`depths` if a Pauli flow exists; otherwise a result with
 * `ok == false`.
 */
PauliFlowResult findPauliFlow(const MBQC_Graph& g);

/// Serializes a PauliFlowResult to JSON (`ok`, `corrf`, `oddNcorrf` as string-keyed objects of vertex-id arrays, and `depths`).
json PauliFlowResultToJson(const PauliFlowResult& result);

/**
 * @brief Rewrites `flow`'s correction sets in place into the *focused* Pauli
 * flow for `g`: an equivalent flow where every correction set only ever
 * contains vertices that are not yet measured relative to the flow's
 * measurement order, so corrections can be applied eagerly as vertices are
 * measured (see Simulator::step()).
 *
 * Implements Definition 4.3 in <http://arxiv.org/abs/2109.05654>, "Relating Measurement Patterns to Circuits via Pauli Flow" (see also Definition 2.5 in
 * <https://arxiv.org/abs/2410.23439>).
 *
 * @param flow The flow to focus, modified in place. Must have `flow.ok == true`.
 * @param g The graph the flow was computed for.
 */
void focus(PauliFlowResult& flow, const MBQC_Graph& g);

#endif
