#ifndef ZXGRAPH_HPP
#define ZXGRAPH_HPP

#include "Quantum_Circuit.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

/// The kind of a ZXGraph vertex ("spider"): a green Z-spider, a red X-spider, or a boundary INPUT/OUTPUT node.
enum class SpiderType { Z, X, INPUT, OUTPUT };
/// The kind of a ZXGraph edge: a normal wire, or a Hadamard edge (a wire with an implicit H gate on it).
enum class EdgeType { SIMPLE, HADAMARD };

/// Converts a SpiderType to its string name (`"Z"`, `"X"`, `"INPUT"`, `"OUTPUT"`).
std::string spiderTypeToString(SpiderType s);

/// A single ZX-calculus vertex: its type, phase (in radians; meaningless for INPUT/OUTPUT), and its incident edges keyed by neighbor id.
struct Spider {
    int id;
    SpiderType type;
    double phase;
    std::unordered_map<int, EdgeType> neighbors; // Map of connected spiders
};

/**
 * @brief A ZX-calculus diagram: a graph of Z/X "spiders" connected by simple
 * or Hadamard edges.
 *
 * Circuits are **not** translated to MBQC patterns through this anymore —
 * Circ2MBQC.hpp's `CIRCtoMBQCGraph()` translates a QuantumCircuit directly
 * into an MBQC_Graph. ZXGraph (and its conversions to/from MBQC_Graph in
 * MBQC2ZX.hpp/ZX2MBQC.hpp) exists for the frontend's `/ZX` diagram view, and
 * as a comparison oracle in tests: converting two graphs to ZX and checking
 * they denote the same linear map (`compareTensors()` in
 * `backend/include/test_helpers.hpp`, via PyZX tensor contraction) verifies
 * that a rewrite or conversion elsewhere preserved semantics.
 *
 * The `toGH()`/`spiderSimplification()`/`handleInputs()`/`handleOutputs()`
 * methods bring a circuit-derived diagram into the "green, Hadamard-only,
 * boundary-isolated" normal form that ZXtoMBQCGraph() (ZX2MBQC.hpp) expects.
 */
class ZXGraph {
private:
    int next_spider_id = 0;

public:
    /// All spiders in this diagram, keyed by id.
    std::unordered_map<int, Spider> spiders;
    /// `(id1, id2, type)` triples, one per edge, kept in parallel with each Spider's own `neighbors` map.
    using Edge = std::tuple<int, int, EdgeType>;
    std::vector<Edge> edges;

    /// Adds a new spider of the given type/phase and returns its (freshly allocated) id.
    int addSpider(SpiderType type, double phase = 0.0);
    /// Adds an edge of the given type between two existing spiders (undirected; updates both spiders' neighbor maps and the `edges` list).
    void addEdge(int id1, int id2, EdgeType type = EdgeType::SIMPLE);
    /// Removes the edge between two spiders, if present.
    void removeEdge(int id1, int id2);
    /// Prints a human-readable dump of the diagram to stdout.
    void printGraph() const;
    /**
     * @brief Builds the ZX diagram corresponding to a quantum circuit, gate by
     * gate: one input/output spider pair per qubit wire, with each supported
     * gate (H, Pauli, S/T (+ their daggers), Rx/Rz, Y/Ry, CX, CZ, CCX/CCZ)
     * appended as its standard ZX-calculus gadget.
     */
    static ZXGraph fromQuantumCircuit(const QuantumCircuit& qc);
    /// The ids of all `INPUT`-type spiders.
    std::vector<int> getInputs() const;
    /// The ids of all `OUTPUT`-type spiders.
    std::vector<int> getOutputs() const;
    /// Serializes the diagram (spiders as `[type, phaseString]` pairs, edges as `[id1, id2, edgeTypeInt]` triples, plus inputs/outputs) to JSON.
    json toJson() const;
    /// Returns a deep copy of this diagram (spiders/edges/next id).
    ZXGraph clone() const;
    /// Inserts a Hadamard-Z-Hadamard chain between two existing spiders (i.e. converts their connection into an explicit `H - Z(0) - H` path). Returns the id of the inserted Z spider.
    int insert_HZH(int id1, int id2); // Takes two node ids and inserts HADAMARD Z HADAMARD between them. Returns the ID of the inserted Z node

    // Transformation to MBQC:
    /// Converts every X-spider into a Z-spider, toggling the edge type (simple <-> Hadamard) on each of its incident edges to preserve semantics ("red to green").
    void toGH();
    /// Fuses two Z-spiders connected by a simple edge into one: rewires `removeId`'s other edges onto `keepId`, sums their phases, and removes `removeId`. (A same-type-different-edge collision is resolved by keeping the edge simple and adding π to `keepId`'s phase.)
    void fuseSpiders(int keepId, int removeId);
    /// Repeatedly fuses every pair of simple-edge-connected Z-spiders until none remain (maximal green spider fusion).
    void spiderSimplification();
    /// Ensures every INPUT spider connects to the rest of the diagram via a Hadamard edge, inserting an `H-Z(0)-H` gadget (insert_HZH()) wherever it currently doesn't.
    void handleInputs();
    /// Ensures every OUTPUT spider connects to the rest of the diagram via a Hadamard edge, inserting an `H-Z(0)-H` gadget (insert_HZH()) wherever it currently doesn't.
    void handleOutputs();

    // Compare tensors:
    /// Serializes the diagram to the PyZX JSON graph format, for cross-checking against `pyzx` (see `backend/test/compare_tensors.py`).
    json toPyZXJson() const;

};

#endif
