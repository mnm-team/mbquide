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


/// Which graph-rewrite rule a GraphRewriteStep applied.
enum class GraphRewriteRuleType {
    LocalComplementation,
    Pivot
};

/// One applied step of greedyOptimizeEdges(): which rule was used, on which vertex/vertices
/// (v is unused, i.e. -1, for LocalComplementation), and the edge-count score it achieved.
struct GraphRewriteStep {
    GraphRewriteRuleType rule;
    int u;
    int v;
    int score;
};


/**
 * @brief A measurement-based quantum computing (MBQC) graph: a graph state
 * (vertices with edges representing CZ-entanglement) where every non-output
 * vertex carries a measurement basis/angle, and outputs carry a Pauli-frame
 * OutputAdjustmentMap instead.
 *
 * This is the backend's central data structure — the MBQC pattern that the
 * REST API's `/api/graph` endpoints read and mutate, that findPauliFlow()
 * analyzes, that Simulator executes, and that MBQCtoZXGraph()/
 * ZXtoMBQCGraph() convert to and from ZX-calculus diagrams. It exposes:
 *  - construction and basic graph/measurement queries,
 *  - the semantics-preserving rewrite **operations** used by the graphical
 *    editor (local complementation, pivot, Z-insertion/deletion, relabeling,
 *    YZ-(un)fusion) — see e.g. localComplementation(), pivot(),
 *  - **automatic simplification** built on those operations (simplify(),
 *    greedyOptimizeEdges()), and
 *  - **JSON (de)serialization** for the REST API, plus a legacy PyZX JSON
 *    import/export pair.
 */
class MBQC_Graph {
public:
    /// Default constructor (an empty 0-vertex graph), so MBQC_Graph can be used as e.g. `std::map<K, MBQC_Graph>::operator[]`'s default value.
    MBQC_Graph() : MBQC_Graph(0, {}, {}) {}

    /**
     * @brief Constructs a graph with `numNodes` vertices (ids `0..numNodes-1`)
     * and no edges. Output vertices are initialized with `OUTPUT` measurement
     * basis and a fresh (standard) OutputAdjustmentMap; all other vertices
     * start with no measurement set.
     */
    MBQC_Graph(int numNodes, const std::vector<int>& inputVertices, const std::vector<int>& outputVertices);

    /// Adds an undirected edge between `u` and `v` (a no-op if either index is out of range).
    void addEdge(int u, int v);

    /**
     * @brief Sets a non-output vertex's measurement basis and angle.
     * Validates that `node` is in range, isn't an output, and — for the
     * single-axis bases `X`/`Y`/`Z` — that `angle` is 0 or π; violations are
     * reported to stderr and the call is a no-op.
     */
    void setMeasurement(int node, MeasurementBasis basis, double angle = 0);
    /// Sets an output vertex's accumulated Pauli-frame correction. Validates that `node` is actually an output.
    void setOutputAdjustment(int node, OutputAdjustmentMap oam);

    /// All edges as `(u, v)` pairs with `u < v`.
    std::vector<std::pair<int, int>> getAllEdges() const;
    /// The neighbors of `u`.
    std::vector<int> getNeighbors(int u) const;
    /// The odd neighborhood of vertex set `S`: vertices with an odd number of neighbors in `S` (GF(2) sum of each `S` member's neighbor set). Used by Pauli flow correction-set computation.
    std::unordered_set<int> oddNeighborhood(const std::unordered_set<int>& S) const;
    /// Direct access to the raw adjacency matrix (`adjacencyMatrix[u][v] != 0` iff `u`-`v` is an edge).
    const std::vector<std::vector<int>>& getAdjacencyMatrix() const;
    /// The number of vertices.
    const int getSize() const;
    /// All output vertices' OutputAdjustmentMaps, keyed by vertex id.
    const std::map<int, OutputAdjustmentMap>& getOutputAdjustments() const;
    /// Mutable access to output vertex `u`'s OutputAdjustmentMap (by reference, so e.g. `graph.getOutputAdjustment(u).adjustOutput("X")` works in place — needed by Simulator).
    OutputAdjustmentMap& getOutputAdjustment(int u);  // needs to be call by ref in order to call: graph.getOutputAdjustment(u).adjustOutput("X"); (for simulator)
    /// @overload Read-only (by-value) version.
    OutputAdjustmentMap getOutputAdjustment(int u) const;

    /// `node`'s `(basis, angle)` measurement pair.
    std::pair<MeasurementBasis, double> getMeasurement(int node) const;

    /// Prints a human-readable dump of the graph (size, inputs/outputs, adjacency matrix, measurements, output adjustments) to stdout.
    void printGraph() const;
    /// Encodes the full graph state (size, adjacency, measurement bases/angles, output adjustments) into a compact deterministic string. Used by simplify()'s fixed-point/cycle detection.
    std::string stateHash() const;

    /// Returns a deep copy of this graph.
    MBQC_Graph clone() const;

    /// Whether `u` is one of this graph's input vertices.
    bool isInput(int u) const;
    /// Whether `u` is one of this graph's output vertices (consistency-checked against `u`'s measurement basis being `OUTPUT`; a mismatch is reported to stderr).
    bool isOutput(int u) const;

    /// The output vertex ids.
    std::vector<int> getOutputs() const;
    /// The input vertex ids.
    std::vector<int> getInputs() const;
    /// All vertex ids that are *not* outputs.
    std::vector<int> getNonOutputs() const;
    /// All vertex ids that are *not* inputs.
    std::vector<int> getNonInputs() const;
    /// All "measurement vertices", i.e. every non-output vertex (inputs included, since inputs also carry a measurement basis).
    std::vector<int> mvertices() const;


    // Operations:

    /**
     * @brief Applies local complementation about vertex `u`: toggles every
     * edge among `u`'s neighbors (present <-> absent), and updates `u`'s own
     * measurement basis/angle and each neighbor's basis/angle to preserve the
     * pattern's semantics (a no-op on `u`'s input status is required — LC on
     * an input vertex is rejected). One of the two fundamental
     * semantics-preserving graph-state rewrite rules (with pivot()).
     */
    void localComplementation(int u);
    /// Applies a pivot on edge `(u, v)`: equivalent to `localComplementation(u); localComplementation(v); localComplementation(u);`. Requires `u`-`v` to be an existing edge.
    void pivot(int u, int v);
    /// Inserts a fresh `Z(0)` vertex connected to every vertex in `vertices`. Used both directly (as a building-mode operation) and internally by lcompRewrite() to realize partial-neighborhood ("nu-set") rewrites via vertex unfusion.
    void ZInsertion(const std::vector<int>& inputVertices);
    /// Removes a Pauli `Z`/`XZ`/`YZ`-basis vertex at angle 0 or π, folding its effect (a possible π phase kick) into each neighbor's measurement/output-adjustment and renumbering all higher vertex ids down by one. Rejected for output vertices or vertices not in a Z-family basis at a Pauli angle.
    void ZDeletion(int u);
    /// @overload Deletes multiple vertices at once, sorted descending first so earlier deletions don't invalidate later indices.
    void ZDeletion(std::vector<int> nodes);
    /// Relabels a Pauli-angle (0, π/2, π, or 3π/2) `XY`/`XZ`/`YZ`-basis vertex to the corresponding single-axis Pauli basis (`X`/`Y`/`Z`), e.g. `XY(0) -> X(0)`, `XY(π/2) -> Y(0)`.
    void relabel(int u);
    /// Relabels a single-axis Pauli-basis (`X`/`Y`/`Z`) vertex into `preferredBasis` (one of `XY`/`XZ`/`YZ`), adjusting its angle so the measurement is unchanged. Rejected if `basis_u` can't be expressed in `preferredBasis`'s plane (e.g. relabeling `Z` into `XY`).
    void relabelPlanar(int u, MeasurementBasis preferredBasis);
    /// @overload Picks a default target plane for `u`'s current basis (`X -> XZ`, `Y -> YZ`, `Z -> XZ`).
    void relabelPlanar(int u);
    /// Merges two non-adjacent `YZ`-basis vertices with identical neighborhoods into one, summing their angles (keeps `u`, removes `v`). See https://doi.org/10.1103/PhysRevA.102.022406.
    void mergeYZ(int u, int v);
    /// Adds a new `YZ`-basis vertex connected only to `u` (an `XY`-basis vertex), the inverse of a YZ-pendant absorption: `u`'s angle becomes `beta`, and the new vertex's angle becomes `beta - u`'s old angle.
    void YZUnfusion(int u, double beta);

    // Simplification:

    /**
     * @brief Repeatedly applies a fixed sequence of simplification passes —
     * relabeling eligible Pauli-angle planar vertices to single-axis basis,
     * local complementation on every `Y` vertex, pivoting `X` vertices with a
     * non-input neighbor, Z-deletion of eligible Pauli-angle Z-family
     * vertices, and YZ-node merging (mergeAllYZNodes()) — until the graph
     * state stops changing (detected via stateHash()) or `maxIterations` is
     * reached.
     */
    void simplify(int maxIterations = 1000);
    /**
     * @brief Scans all pairs of YZ-basis vertices and merges any eligible pair
     * (mergeYZ()), and absorbs any YZ vertex whose only neighbor is an XY
     * vertex back into that neighbor (undoing a YZUnfusion()), repeating until
     * no more merges/absorptions apply.
     * @return Whether at least one merge/absorption was performed.
     */
    bool mergeAllYZNodes();
    /**
     * @brief Greedily reduces the graph's edge count by alternating local
     * complementation and pivot, each considered both on full neighborhoods
     * and on greedily-searched partial ("nu-set") neighborhoods realized via
     * vertex unfusion, always applying whichever candidate rewrite scores
     * highest. Mirrors `greedy_optimize_edges()` from the project's
     * `pattern_optimize.py` reference implementation.
     * @param favorVertexRemoval If true, a zero-score rewrite is still applied
     * when it would expose a vertex for Z-deletion (net vertex-count
     * reduction even without an edge-count win).
     * @return The sequence of rewrite steps that were applied.
     */
    std::vector<GraphRewriteStep> greedyOptimizeEdges(bool favorVertexRemoval = true);

    /// Whether greedyOptimizeEdges() would apply at least one rewrite if called right now.
    /// Runs the real algorithm on a throwaway clone, so it stays exactly in sync with
    /// greedyOptimizeEdges() instead of duplicating its eligibility logic. Intended for UI
    /// enablement checks where actually mutating the graph isn't wanted.
    bool canOptimizeEdges(bool favorVertexRemoval = true) const;

    // JSON:

    /// Serializes the graph to the REST API's JSON shape: `size`, `inputs`, `outputs`, `meas` (per-vertex `[basisString, angleString]`), `edges`, and `outAdj` (per-output OutputAdjustmentMap JSON).
    json toJson() const;
    /// Exports the graph to the legacy PyZX JSON vertex/edge format, laid out in a grid `rowLength` vertices wide, for interop/debugging.
    void exportToPYZXJsonFile(const std::string& filename, int rowLength = 4) const;
    /// Inverse of toJson().
    static MBQC_Graph fromJson(const json& j);
    /// Inverse of exportToPYZXJsonFile().
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
