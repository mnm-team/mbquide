#include "MBQC2ZX.hpp"

ZXGraph MBQCtoZXGraph(MBQC_Graph mbqc) {
    ZXGraph zx;

    std::unordered_map<int, int> mbqcToZxSpider; // MBQC node id -> ZX spider id

    // Step 1: Add Z-spiders for each MBQC node and add Inputs and Outputs
    for (int i = 0; i < mbqc.getSize(); ++i) {
        int spiderId = zx.addSpider(SpiderType::Z);
        mbqcToZxSpider[i] = spiderId;

        if (mbqc.isInput(i)) {
            int input = zx.addSpider(SpiderType::INPUT);
            zx.addEdge(mbqcToZxSpider[i], input, EdgeType::SIMPLE);
        }
        
        if (mbqc.isOutput(i)) {
            OutputAdjustmentMap oam = mbqc.getOutputAdjustment(i);
            
            ZXGraph outAdjZX = ZXGraph::fromQuantumCircuit(oam.toCircuit());
            
            if (outAdjZX.spiders.size() == 2) {  // Only Input and Output -> No output adjustment needed
                int output = zx.addSpider(SpiderType::OUTPUT);
                zx.addEdge(mbqcToZxSpider[i], output, EdgeType::SIMPLE);
            } else if (outAdjZX.spiders.size() >= 2) {
                int lastAddition = mbqcToZxSpider[i];
                for (int outID = 1; outID < outAdjZX.spiders.size(); ++outID) {
                    Spider addOut = outAdjZX.spiders.at(outID);
                    SpiderType st = addOut.type;
                    double phase = addOut.phase;
                    EdgeType et = addOut.neighbors.at(outID-1);
                    
                    int addition = zx.addSpider(st, normalize_radians(phase));
                    zx.addEdge(lastAddition, addition, et);
                    lastAddition = addition;
                }
            } else {
                std::cerr << "Conversion MBQC -> ZX: OutAdjZX graph has less than two nodes, so no input and output! This should not happen.\n";

            }
        }
    }

    // Step 2: Add Hadamard edges for each edge in MBQC graph
    const auto& adj = mbqc.getAdjacencyMatrix();
    int n = adj.size();
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (adj[u][v] != 0) {
                zx.addEdge(mbqcToZxSpider[u], mbqcToZxSpider[v], EdgeType::HADAMARD);
            }
        }
    }


    // Step 3: Add spiders for measurements
    for (int i = 0; i < mbqc.getSize(); ++i) {
        auto [basis, angle] = mbqc.getMeasurement(i);
        angle = normalize_radians(angle);
        int measSpiderId;
        int secondMeasSpiderId;
        switch (basis) {
            case MeasurementBasis::XY:
                measSpiderId = zx.addSpider(SpiderType::Z, normalize_radians(-angle));
                break;
            case MeasurementBasis::XZ:
                measSpiderId = zx.addSpider(SpiderType::Z, M_PI/2);
                secondMeasSpiderId = zx.addSpider(SpiderType::X, normalize_radians(angle));
                zx.addEdge(measSpiderId, secondMeasSpiderId, EdgeType::SIMPLE);
                break;
            case MeasurementBasis::YZ:
                measSpiderId = zx.addSpider(SpiderType::X, normalize_radians(angle));
                break;
            case MeasurementBasis::X:
                if (fAlmostEqual(0, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::Z, 0);
                } else if (fAlmostEqual(M_PI, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::Z, M_PI);
                } else {
                    std::cerr << "Conversion MBQC -> ZX: Angle not 0 or pi for Measurement X, instead " << radiansToString(angle) << "!\n";
                }
                break;
            case MeasurementBasis::Y:
                if (fAlmostEqual(0, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::Z, normalize_radians(-M_PI/2));
                } else if (fAlmostEqual(M_PI, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::Z, M_PI/2);
                } else {
                    std::cerr << "Conversion MBQC -> ZX: Angle not 0 or pi for Measurement Y, instead " << radiansToString(angle) << "!\n";
                }
                break;
            case MeasurementBasis::Z:
                if (fAlmostEqual(0, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::X, 0);
                } else if (fAlmostEqual(M_PI, angle)) {
                    measSpiderId = zx.addSpider(SpiderType::X, M_PI);
                } else {
                    std::cerr << "Conversion MBQC -> ZX: Angle not 0 or pi for Measurement Z, instead " << radiansToString(angle) << "!\n";
                }
                break;
            case MeasurementBasis::OUTPUT:
                measSpiderId = zx.addSpider(SpiderType::Z, 0);  // Unnecessary: keeping for the addEdge command below
                break;
            default:
                std::cerr << "Conversion MBQC -> ZX: Unkwown Measurement Basis!\n";
        }

        zx.addEdge(measSpiderId, mbqcToZxSpider[i], EdgeType::SIMPLE);
    }


    return zx;
}