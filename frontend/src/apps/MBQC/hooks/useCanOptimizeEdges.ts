import { useEffect, useMemo, useState } from 'react';
import { NodeType, Edge } from '../types';
import { checkCanOptimizeEdges } from '../api/graphApi';

// Whether greedyOptimizeEdges() would currently change the graph, asked from the backend since
// that's the only place the real LC/pivot/nu-set eligibility logic lives (see
// MBQC_Graph::canOptimizeEdges()). Re-checks whenever the graph's measurement/topology content
// changes, but not on node-position-only updates (e.g. dragging), which don't affect the answer.
export const useCanOptimizeEdges = (
  nodes: NodeType[],
  edges: Edge[],
  inputs: number[],
  outputs: number[]
): boolean => {
  const [canOptimizeEdges, setCanOptimizeEdges] = useState(false);

  const topologyFingerprint = useMemo(() => {
    const meas = nodes
      .map((n) => `${n.id}:${n.basis}:${n.phase ?? ''}`)
      .sort()
      .join(',');
    const edgeList = edges
      .map((e) => `${e.source}-${e.target}`)
      .sort()
      .join(',');
    return `${meas}|${edgeList}|${[...inputs].sort()}|${[...outputs].sort()}`;
  }, [nodes, edges, inputs, outputs]);

  useEffect(() => {
    let cancelled = false;

    checkCanOptimizeEdges()
      .then((result) => {
        if (!cancelled) setCanOptimizeEdges(result);
      })
      .catch((error) => {
        console.error('Error checking optimizeEdges availability:', error);
        if (!cancelled) setCanOptimizeEdges(false);
      });

    return () => {
      cancelled = true;
    };
  }, [topologyFingerprint]);

  return canOptimizeEdges;
};
