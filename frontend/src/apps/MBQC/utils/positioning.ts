import { NodeType, Edge } from '../types';

export const LAYOUT_CONFIG = {
  H_GAP: 200, // horizontal spacing between nodes
  V_GAP: 100, // vertical spacing between nodes
  START_OFFSET_X: 100,
  START_OFFSET_Y: 100,
  FLOW_CENTER_X: 900,
  FLOW_CENTER_Y: 600,
} as const;


export const getNodePosition = (
  nodes: NodeType[],
  edges: Edge[],
  inputs: number[],
): Record<number, { x: number; y: number }> => {
  // Build adjacency list for UNDIRECTED graph
  const adjacency = new Map<number, number[]>();
  for (const edge of edges) {
    if (!adjacency.has(edge.source)) adjacency.set(edge.source, []);
    if (!adjacency.has(edge.target)) adjacency.set(edge.target, []);
    adjacency.get(edge.source)!.push(edge.target);
    adjacency.get(edge.target)!.push(edge.source);
  }

  // BFS from input nodes, each node assigned to the FIRST layer it's reached
  // (visited set prevents back-traversal from corrupting layer assignments)
  const layerOf = new Map<number, number>();
  const queue: { id: number; layer: number }[] = [];

  for (const inputId of inputs) {
    queue.push({ id: inputId, layer: 0 });
    layerOf.set(inputId, 0);
  }

  let head = 0;
  while (head < queue.length) {
    const { id, layer } = queue[head++];

    for (const neighbor of adjacency.get(id) ?? []) {
      if (!layerOf.has(neighbor)) { // ← only visit each node once
        layerOf.set(neighbor, layer + 1);
        queue.push({ id: neighbor, layer: layer + 1 });
      }
    }
  }

  // Fallback: assign any unreachable nodes to layer 0
  for (const node of nodes) {
    if (!layerOf.has(node.id)) layerOf.set(node.id, 0);
  }

  // Group nodes by layer
  const layers = new Map<number, number[]>();
  for (const [id, layer] of layerOf) {
    if (!layers.has(layer)) layers.set(layer, []);
    layers.get(layer)!.push(id);
  }

  // Compute x/y positions
  const positionOf: Record<number, { x: number; y: number }> = {};

  for (const [layer, nodeIds] of layers) {
    const x = LAYOUT_CONFIG.START_OFFSET_X + layer * LAYOUT_CONFIG.H_GAP;
    nodeIds.forEach((id, index) => {
      const y = LAYOUT_CONFIG.START_OFFSET_Y + index * LAYOUT_CONFIG.V_GAP;
      positionOf[id] = { x, y };
    });
  }

  return positionOf;
};



export const positionNodes = (
  nodes: NodeType[],
  edges: Edge[],
  inputs: number[],
): NodeType[] => {

  const positions = getNodePosition(nodes, edges, inputs);

  return nodes.map((node) => ({
    ...node,
    x: positions[node.id]?.x ?? node.x,
    fx: positions[node.id]?.x ?? node.x,
    y: positions[node.id]?.y ?? node.y,
    fy: positions[node.id]?.y ?? node.y,
  }));
};


export const computeDepthBasedPositions = (
  nodes: NodeType[],
  edges: Edge[],
  depths: number[],
): number[][] => {

  const maxDepth = Math.max(...depths);

  // Group node ids by depth
  const nodesByDepth: number[][] = Array.from({ length: maxDepth + 1 }, () => []);
  nodes.forEach(node => {
    nodesByDepth[depths[node.id]].push(node.id);
  });

  // Build undirected adjacency for barycenter calculations
  const adjacency = new Map<number, number[]>();
  for (const edge of edges) {
    if (!adjacency.has(edge.source)) adjacency.set(edge.source, []);
    if (!adjacency.has(edge.target)) adjacency.set(edge.target, []);
    adjacency.get(edge.source)!.push(edge.target);
    adjacency.get(edge.target)!.push(edge.source);
  }

  // --- Barycenter crossing minimization (Sugiyama-style) ---
  // For each node, compute the average index of its neighbors in the adjacent layer,
  // then sort the current layer by that average ("barycenter").
  // Repeat sweeping left→right then right→left for several iterations.

  const getBarycenter = (id: number, referenceLayer: number[]): number => {
    const indexInRef = new Map(referenceLayer.map((nid, i) => [nid, i]));
    const neighbors = (adjacency.get(id) ?? []).filter(n => indexInRef.has(n));
    if (neighbors.length === 0) return Infinity; // push unconnected nodes to the end
    return neighbors.reduce((sum, n) => sum + indexInRef.get(n)!, 0) / neighbors.length;
  };

  const ITERATIONS = 4;
  for (let iter = 0; iter < ITERATIONS; iter++) {
    // Forward pass: sort each layer by barycenter of previous layer
    for (let d = 1; d <= maxDepth; d++) {
      const ref = nodesByDepth[d - 1];
      nodesByDepth[d].sort((a, b) => getBarycenter(a, ref) - getBarycenter(b, ref));
    }
    // Backward pass: sort each layer by barycenter of next layer
    for (let d = maxDepth - 1; d >= 0; d--) {
      const ref = nodesByDepth[d + 1];
      nodesByDepth[d].sort((a, b) => getBarycenter(a, ref) - getBarycenter(b, ref));
    }
  }

  return nodesByDepth;

};


export const getDepthOrderedNodes = (
  nodes: NodeType[],
  edges: Edge[],
  depths: number[],
  corrf: Record<number, number[]>,
  oddNcorrf: Record<number, number[]>,
  ): NodeType[] => {
    
  const depthPositions = computeDepthBasedPositions(nodes, edges, depths)
  const maxDepth = Math.max(...depths);


    // --- Position assignment (same as before) ---
  const maxHeight = Math.max(...depthPositions.map(arr => arr.length));
  const startX = LAYOUT_CONFIG.FLOW_CENTER_X + 0.5 * maxDepth * LAYOUT_CONFIG.H_GAP;
  const startY = LAYOUT_CONFIG.FLOW_CENTER_Y - 0.5 * maxHeight * LAYOUT_CONFIG.V_GAP;

  const orderedNodes: NodeType[] = [];
  depthPositions.forEach((nodeIndices, depth) => {
    const xPos = startX - depth * LAYOUT_CONFIG.H_GAP;
    const layerHeight = (nodeIndices.length - 1) * LAYOUT_CONFIG.V_GAP;
    const layerStartY = startY - layerHeight / 2;

    nodeIndices.forEach((id, depthIndex) => {
      const originalNode = nodes.find(node => node.id === id);
      if (!originalNode) {
        console.error(`Node with id ${id} not found`);
        return;
      }
      const correctionSet: number[] = corrf[id] ?? [];
      const oddCorrectionSet: number[] = oddNcorrf[id] ?? [];
      orderedNodes.push({
        id,
        basis: originalNode.basis,
        phase: originalNode.phase,
        x: xPos,
        y: depthIndex * LAYOUT_CONFIG.V_GAP + layerStartY,
        fx: xPos,
        fy: depthIndex * LAYOUT_CONFIG.V_GAP + layerStartY,
        correctionSet,
        oddCorrectionSet,
      });
    });
  });

  return orderedNodes;

};



export const getCenterOfNodes = (nodes: NodeType[]): { x: number; y: number } => {
  if (nodes.length === 0) return { x: 1850, y: 5000 };

  const sum = nodes.reduce(
    (acc, node) => ({
      x: acc.x + (node.x ?? 0),
      y: acc.y + (node.y ?? 0),
    }),
    { x: 0, y: 0 }
  );

  return {
    x: sum.x / nodes.length,
    y: sum.y / nodes.length,
  };
};