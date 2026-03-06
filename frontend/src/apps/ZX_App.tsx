import { ZX_Graph } from '../components/Graph'
import LoadingOverlay from '../components/LoadingOverlay'
import { useState, useEffect, useCallback } from 'react'
import { useNavigate } from "react-router-dom";
import { LAYOUT_CONFIG } from './MBQC/utils/positioning';
import { ControlPanel } from '../components/ControlPanel';

type GraphApiResponse = {
  nodes: [string, string][]
  edges: [number, number, number][]
  size: number
  inputs: number[]
  outputs: number[]
}

type NodeType = {
  id: number
  basis: string
  phase?: string
  x?: number
  y?: number
  fx?: number | null
  fy?: number | null
}

type Edge = {
  source: number
  target: number
  colorCode: number
}



// ###### UTILITIES FOR POSITIONONG NODES #######
// Directed adjacency list
const buildDirectedAdjList = (edges: [number, number][], size: number): Map<number, number[]> => {
  const adj = new Map<number, number[]>();
  for (let i = 0; i < size; i++) adj.set(i, []);

  for (const [src, tgt] of edges) {
    adj.get(src)!.push(tgt); // only src ➜ tgt
  }

  return adj;
};

const computeLevels = (
  inputs: number[],
  outputs: number[],
  edges: [number, number][],
  size: number
): Record<number, number> => {
  const forwardAdj = buildDirectedAdjList(edges, size);
  const reverseAdj = buildDirectedAdjList(edges.map(([src, tgt]) => [tgt, src]), size);

  const bfsLevels = (
    starts: number[],
    adj: Map<number, number[]>
  ): Record<number, number> => {
    const levelMap: Record<number, number> = {};
    const queue: [number, number][] = [];

    for (const id of starts) {
      queue.push([id, 0]);
      levelMap[id] = 0;
    }

    while (queue.length > 0) {
      const [node, level] = queue.shift()!;
      for (const neighbor of adj.get(node) || []) {
        if (!(neighbor in levelMap)) {
          levelMap[neighbor] = level + 1;
          queue.push([neighbor, level + 1]);
        }
      }
    }

    return levelMap;
  };

  const fromInputs = bfsLevels(inputs, forwardAdj);
  const fromOutputs = bfsLevels(outputs, reverseAdj);

  // Final level map: average the two, fallback if only one side is reachable
  const levelMap: Record<number, number> = {};
  for (let i = 0; i < size; i++) {
    const inLevel = fromInputs[i];
    const outLevel = fromOutputs[i];

    if (inLevel !== undefined && outLevel !== undefined) {
      levelMap[i] = Math.round((inLevel + (fromOutputs[i] ?? 0)) / 2);
    } else {
      levelMap[i] = inLevel ?? outLevel ?? 0;
    }
  }

  return levelMap;
};



// ############################################################


export default function ZX_App() {
  const navigate = useNavigate();

  // Graph state
  const [nodes, setNodes] = useState<NodeType[]>([])
  const [edges, setEdges] = useState<Edge[]>([])
  const [inputs, setInputs] = useState<number[]>([])
  const [outputs, setOutputs] = useState<number[]>([])

  // Loading flag
  const [loading, setLoading] = useState<boolean>(false)

  const fetchGraph = useCallback(async () => {
    setLoading(true)

    const res = await fetch('http://localhost:18080/api/qasm', {
      method: 'GET',
      headers: { 'Content-Type': 'application/json' },
      credentials: "include",
    })
    
    if (!res.ok) {
      console.error('Server returned', res.status, res.statusText)
      setLoading(false)
      return
    }
    
    const data: GraphApiResponse = await res.json()
    console.log(data)


    const levelMap = computeLevels(data.inputs, data.outputs, data.edges, data.size);

    // Group nodes by their level
    const levelGroups: Record<number, number[]> = {};
    for (let i = 0; i < data.size; i++) {
      const level = levelMap[i] ?? 0;
      if (!levelGroups[level]) levelGroups[level] = [];
      levelGroups[level].push(i);
    }

    const nodePositions: Record<number, { x: number; y: number }> = {};
    Object.entries(levelGroups).forEach(([levelStr, nodeIds]) => {
      const level = parseInt(levelStr);
      nodeIds.forEach((id, i) => {
        nodePositions[id] = {
          x: i * LAYOUT_CONFIG.H_GAP + 100,
          y: level * LAYOUT_CONFIG.V_GAP + 100,
        };
      });
    });

    const newNodes: NodeType[] = data.nodes.map(([basis, phase], idx) => {
      const pos = nodePositions[idx];
      return {
        id: idx,
        basis,
        phase: phase || undefined,
        x: pos?.x,
        y: pos?.y,
        fx: pos?.x,
        fy: pos?.y,
      }
    })

    const newEdges: Edge[] = data.edges.map(([src, tgt, col]) => ({
      source: src,
      target: tgt,
      colorCode: col,
    }))

    setNodes(newNodes)
    setEdges(newEdges)
    setInputs(data.inputs)
    setOutputs([])
    setLoading(false)
  }, [])


  const toMBQC = useCallback(async () => {
    setLoading(true)

    const res = await fetch('http://localhost:18080/api/qasm', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ "transform": true }),
      credentials: "include",
    })
    
    if (!res.ok) {
      console.error('Server returned', res.status, res.statusText)
      setLoading(false)
      return
    }

    navigate("/MBQC")
  
  }, [navigate])


  useEffect(() => {
    fetchGraph()
  }, [fetchGraph])

  return (
    <div style={{ textAlign: 'center', padding: 20 }}>
      <>
        <LoadingOverlay isLoading={loading} />

        <ZX_Graph
          nodes={nodes}
          edges={edges}
          inputs={inputs}
          outputs={outputs}
        />

        <ControlPanel
          onTransformToMBQC={toMBQC}
        />
      </>
    </div>
  )
}
