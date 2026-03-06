#include "ZX2MBQC.hpp"

MBQC_Graph ZXtoMBQCGraph(ZXGraph original_zx, bool planarOnly) {

    ZXGraph zx = original_zx.clone();

    zx.toGH();

    zx.spiderSimplification();

    zx.handleInputs();

    zx.handleOutputs();

    // TODO??: disentangle_outputs(res) #outputs should not be interconnected


    std::map<int, int> zxToMbqc;
    int idx = 0;
    for (const auto& [id, spider] : zx.spiders) {
        zxToMbqc[id] = idx++;
    }

    int numNodes = zxToMbqc.size();
    std::vector<int> inputVertices;
    std::vector<int> outputVertices;

    for (const auto& [id, spider] : zx.spiders) {
        if (spider.type == SpiderType::INPUT) {
            int mbqcId = zxToMbqc[id];
            inputVertices.push_back(mbqcId);
        }
        if (spider.type == SpiderType::OUTPUT) {
            int mbqcId = zxToMbqc[id];
            outputVertices.push_back(mbqcId);
        }
    }

    MBQC_Graph mbqc(numNodes, inputVertices, outputVertices);
    
    // Adding edges
    for (const auto& [id, spider] : zx.spiders) {
        int u = zxToMbqc[id];
        for (const auto& [neighbor, edgeType] : spider.neighbors) {
            if (edgeType == EdgeType::SIMPLE) {
                throw std::runtime_error("Edge is SIMPLE. All edges should be HADAMARD when transforming to MBQC!\n");
            }
            int v = zxToMbqc[neighbor];
            if (u < v) {
                mbqc.addEdge(u, v);
            }
        }
    }

    // Initializing Measurement bases
    for (const auto& [id, spider] : zx.spiders) {
        switch (spider.type) {
            case SpiderType::Z: {
                int node = zxToMbqc[id];
                float phase = normalize_radians(spider.phase);
                MeasurementBasis basis;
                if (!planarOnly && (fAlmostEqual(0, phase) || fAlmostEqual(M_PI, phase))) {
                    basis = MeasurementBasis::X;
                } else {
                    basis = MeasurementBasis::XY;
                }
                mbqc.setMeasurement(node, basis, phase);
                break;
            }
            case SpiderType::INPUT: {
                int node = zxToMbqc[id];
                MeasurementBasis basis = MeasurementBasis::X;
                if (spider.phase != 0) {
                    std::cerr << "Input Spider " << id << " has phase " << spider.phase << ", which is weird and should not happen!\n";
                }
                mbqc.setMeasurement(node, basis, 0);
                break;
            }
            case SpiderType::OUTPUT: {
                int node = zxToMbqc[id];
                MeasurementBasis basis = MeasurementBasis::OUTPUT;
                mbqc.setMeasurement(node, basis, 0);
                break;
            }
            default: {
                std::cerr << "Spider type " << spiderTypeToString(spider.type) << " should not be present in the conversion at this point!\n";
                break;
            }
        }
    }
    
    return mbqc;
}