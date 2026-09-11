#ifndef ZX2MBQC
#define ZX2MBQC

#include "ZX_Graph.hpp"
#include "MBQC_Graph.hpp"

/**
 * @brief Converts a ZXGraph into an equivalent MBQC_Graph.
 *
 * `original_zx` is first cloned and brought into MBQC normal form:
 * ZXGraph::toGH() (all-green), ZXGraph::spiderSimplification() (maximally
 * fused), then ZXGraph::handleInputs()/handleOutputs() (boundary spiders
 * isolated behind Hadamard edges). What remains is exactly a graph state —
 * Z-spiders connected only by Hadamard edges — which maps directly onto an
 * MBQC_Graph: each spider becomes a vertex (renumbered densely by iteration
 * order), each Hadamard edge becomes a graph edge, and each spider's phase
 * becomes its measurement angle in the `X`/`XY` basis (`X` only when
 * `planarOnly` is false and the phase is a multiple of π; `XY` otherwise).
 * INPUT/OUTPUT spiders become the graph's input/output vertices.
 *
 * @param original_zx The ZX diagram to convert (not modified; a clone is
 * transformed internally).
 * @param planarOnly If true, every measurement is kept in the `XY` plane
 * instead of using the `X` basis for π-multiple phases.
 * @return The equivalent MBQC_Graph.
 * @throws std::runtime_error if, after normalization, a non-Hadamard edge or
 * an unexpected spider type remains (indicates the input wasn't a valid
 * circuit-derived diagram).
 */
MBQC_Graph ZXtoMBQCGraph(ZXGraph original_zx, bool planarOnly = false);

#endif
