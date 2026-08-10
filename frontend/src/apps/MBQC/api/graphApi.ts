import { NodeType, Edge, OutputAdjustment, GraphApiResponse } from '../types';

const API_BASE_URL = 'http://localhost:18080/api';

export const fetchGraphFromBackend = async (): Promise<GraphApiResponse> => {
  const response = await fetch(`${API_BASE_URL}/graph`, { 
    credentials: 'include' 
  });
  
  if (!response.ok) {
    throw new Error(`Failed to fetch graph: ${response.statusText}`);
  }
  
  return response.json();
};

export const writeGraphToBackend = async (
  nodes: NodeType[],
  edges: Edge[],
  inputs: number[],
  outputs: number[],
  outAdj: Record<string, OutputAdjustment>
): Promise<void> => {
  const measurements: Record<string, { basis: string; angle: string }> = {};
  
  for (const node of nodes) {
    measurements[node.id.toString()] = {
      basis: node.basis,
      angle: node.phase || "0",
    };
  }

  const edgePairs = edges.map((e) => [e.source, e.target]);

  const graphData = {
    size: nodes.length,
    inputs,
    outputs,
    edges: edgePairs,
    meas: measurements,
    outAdj,
  };

  const response = await fetch(`${API_BASE_URL}/graph`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(graphData),
    credentials: "include",
  });

  if (!response.ok) {
    throw new Error(`Failed to write graph: ${response.statusText}`);
  }

  console.log("Graph successfully sent to backend.");
};

export const executeGraphOperation = async (
  operation: Record<string, any>
): Promise<GraphApiResponse> => {
  return executeAPIOperation("graph", operation);
};

export const executeAPIOperation = async (
  apiExtension: string,
  operation: Record<string, any>
): Promise<GraphApiResponse> => {
  const response = await fetch(`${API_BASE_URL}/${apiExtension}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(operation),
    credentials: "include",
  });

  if (!response.ok) {
    throw new Error(`HTTP error! status: ${response.status}`);
  }

  return response.json();
};

// Read-only: asks the backend whether greedyOptimizeEdges() would actually change the current
// session graph, without applying it. The eligibility logic (LC/pivot costs, nu-sets, Pauli
// relabeling, ...) only exists in the backend, so this is the only way to answer that question
// correctly instead of duplicating (and risking drifting from) that logic on the frontend.
export const checkCanOptimizeEdges = async (): Promise<boolean> => {
  const response = await fetch(`${API_BASE_URL}/graph`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ checkOptimizeEdges: true }),
    credentials: "include",
  });

  if (!response.ok) {
    throw new Error(`Failed to check optimizeEdges: ${response.statusText}`);
  }

  const data = await response.json();
  return Boolean(data.canOptimizeEdges);
};