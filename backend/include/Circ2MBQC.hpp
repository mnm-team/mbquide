#ifndef CIRC2MBQC
#define CIRC2MBQC

#include "Quantum_Circuit.hpp"
#include "MBQC_Graph.hpp"
#include <stdexcept>

//  Translates a QuantumCircuit into an MBQC_Graph
//  following the method of Broadbent & Kashefi (arXiv:quant-ph/0704.1736)
//
//  Gate → pattern rewrites:
//
//    J(α) on qubit q
//      - The current output node for qubit q is assigned measurement XY(-α).
//      - A fresh node is added and becomes the new output for qubit q.
//      - An edge is added between the old output and the fresh node.
//
//    CZ on qubits q0, q1
//      - An edge is added between the current output nodes of q0 and q1.
//      (Both nodes remain outputs; their measurement basis is set later.)
//
//  After all gates are processed the live output nodes become the pattern
//  outputs.
//
//  Identity padding:
//  The MBQC_Graph convention requires inputs ∩ outputs = ∅.  If after
//  building the raw pattern any qubit's input node is still also its output
//  (i.e. no J gate was ever applied to it), an identity pattern
//
//       [input/square] --XY(0)-- [new output/circle]
//
//  is inserted to separate them.
//
//  If planarOnly == true, all X measurements are converted to XY
inline MBQC_Graph CIRCtoMBQCGraph(QuantumCircuit circ, bool planarOnly = true) {
    
    QuantumCircuit tc = circ.transpile();

    const int nq = tc.num_qubits;

    //  We pre-allocate one node per qubit to act as the initial input/output node.
    //  all nodes start as both inputs and outputs
    //  the output set will be updated as we consume gates.

    std::vector<int> wireOut(nq);
    for (int q = 0; q < nq; ++q) wireOut[q] = q;

    int nextNode = nq;

    struct NodeInfo {
        MeasurementBasis basis = MeasurementBasis::OUTPUT; // default
        double angle = 0.0;
    };

    std::vector<NodeInfo> nodes(nq);
    std::vector<std::pair<int,int>> edges;


    for (const auto& g : tc.gates) {
        std::string name = g.name;
        for (auto& c : name) c = static_cast<char>(std::toupper(c));

        if (name == "J") {
            // J(α) on qubit q:
            //   - Assign XY(-α) to the current wire-output node.
            //   - Allocate a fresh node (new output, no measurement yet).
            //   - Connect old output → fresh node.
            int q     = g.qubits.at(0);
            double alpha = g.params.at(0);

            int oldOut = wireOut[q];
            int newOut = nextNode++;

            nodes.push_back(NodeInfo{MeasurementBasis::OUTPUT, 0.0});

            nodes[oldOut].basis = MeasurementBasis::XY;
            nodes[oldOut].angle = -alpha;

            edges.push_back({oldOut, newOut});
            wireOut[q] = newOut;

        } else if (name == "CZ") {
            // CZ on qubits q0, q1:
            //   - Add an edge between their current output nodes.
            int q0 = g.qubits.at(0);
            int q1 = g.qubits.at(1);
            edges.push_back({wireOut[q0], wireOut[q1]});

        } else if (name == "MEASURE") {
            // no graph node added.
        } else {
            // Any gate that was not reduced to {J, CZ} by transpile() is
            // unexpected; surface it as a clear error.
            throw std::invalid_argument("CirctoMBQCGraph: gate '" + g.name + "' was not reduced to {J, CZ} by transpile().");
        }
    }

    //  Inputs  : the original nq nodes (one per qubit wire).
    std::vector<int> inputNodes(nq);
    for (int q = 0; q < nq; ++q) inputNodes[q] = q;
    

    // Insert identity patterns where input == output.
    for (int q = 0; q < nq; ++q) {
        if (wireOut[q] == q) { 
            int midNode = nextNode++;
            int newOut  = nextNode++;
 
            nodes.push_back(NodeInfo{MeasurementBasis::X, 0.0});
            nodes.push_back(NodeInfo{MeasurementBasis::OUTPUT, 0.0});
 
            // input(q, X) — midNode(X) — newOut(output)
            edges.push_back({q,       midNode});
            edges.push_back({midNode, newOut});
 
            // Mark the input node itself as X-measured.
            nodes[q].basis = MeasurementBasis::X;
            nodes[q].angle = 0.0;
 
            wireOut[q] = newOut;
        }
    }


    //  Outputs : the current wire-output nodes after all gates.
    std::vector<int> outputNodes(nq);
    for (int q = 0; q < nq; ++q) outputNodes[q] = wireOut[q];



    // ===== Construct the MBQC_Graph. =====

    const int totalNodes = static_cast<int>(nodes.size());
    MBQC_Graph graph(totalNodes, inputNodes, outputNodes);

    for (const auto& [u, v] : edges) {
        graph.addEdge(u, v);
    }

    for (int n = 0; n < totalNodes; ++n) {
        const auto& ni = nodes[n];
        bool isOut = graph.isOutput(n);

        if (isOut) continue;

        if (ni.basis == MeasurementBasis::OUTPUT) {
            throw std::runtime_error("CirctoMBQCGraph: found output in NodeInfo whereas it was not present in output list!");
        }

        graph.setMeasurement(n, ni.basis, ni.angle);
    }


    if (planarOnly) {
        for (int n = 0; n < graph.getSize(); ++n) {
            if (graph.isOutput(n)) continue;
            auto [b, a] = graph.getMeasurement(n);
            if (b == MeasurementBasis::X) {
                graph.relabelPlanar(n, MeasurementBasis::XY);
            }
        }
    }

    return graph;
}

#endif // CIRC2MBQC