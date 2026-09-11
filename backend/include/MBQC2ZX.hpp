#ifndef MBQC2ZX
#define MBQC2ZX

#include "ZX_Graph.hpp"
#include "MBQC_Graph.hpp"

/**
 * @brief Converts an MBQC_Graph into an equivalent ZXGraph.
 *
 * Each MBQC vertex becomes a Z-spider; each MBQC edge becomes a Hadamard
 * edge between the corresponding spiders (an MBQC graph state is exactly a
 * Hadamard-edge Z-spider graph). Input/output vertices get INPUT/OUTPUT
 * boundary spiders attached, with each output's accumulated
 * OutputAdjustmentMap realized as an extra chain of spiders
 * (OutputAdjustmentMap::toCircuit(), via ZXGraph::fromQuantumCircuit()).
 * Every vertex's measurement (`MeasurementBasis` + angle) is then appended as
 * its own small spider gadget encoding that measurement's effect.
 *
 * @param mbqc The MBQC graph to convert (passed by value; used as a local
 * working copy for reading output adjustments).
 * @return The equivalent ZXGraph.
 */
ZXGraph MBQCtoZXGraph(MBQC_Graph mbqc);

#endif
