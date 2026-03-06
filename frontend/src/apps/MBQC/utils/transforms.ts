import { NodeType, Edge, GraphApiResponse, UpdateGraphOptions } from '../types';

export const transformApiResponseToNodes = (
  data: GraphApiResponse,
): NodeType[] => {
  return (data.meas as [string, string][]).map(([basis, phase], index) => {
    return {
      id: index,
      basis,
      phase: phase || undefined,
      x: 0,
      y: 0,
      fx: 0,
      fy: 0,
    };
  });
};

export const transformApiResponseToEdges = (
  data: GraphApiResponse
): Edge[] => {
  return (data.edges as [number, number][]).map(([source, target]) => ({
    source,
    target,
    colorCode: 1,
  }));
};

export const transformNodesForUpdate = (
  data: GraphApiResponse,
  existingNodes: NodeType[],
  options: UpdateGraphOptions = {}
): NodeType[] => {
  const { pos, deleted_index } = options;
  
  return (data.meas as [string, string][]).map(([basis, phase], index) => {
    // If deleted_index is defined and this index is >= it, shift by +1
    const oldIndex = (deleted_index !== undefined && index >= deleted_index) 
      ? index + 1 
      : index;

    return {
      id: index,
      basis,
      phase: phase || undefined,
      x: existingNodes[oldIndex]?.x ?? pos?.[0] ?? (index * 150) + 285000 / (data.size * 150),
      y: existingNodes[oldIndex]?.y ?? pos?.[1] ?? 500,
      fx: existingNodes[oldIndex]?.fx ?? pos?.[0] ?? (index * 150) + 285000 / (data.size * 150),
      fy: existingNodes[oldIndex]?.fy ?? pos?.[1] ?? 500,
    };
  });
};

export const preserveNodePositions = (
  data: GraphApiResponse,
  existingNodes: NodeType[],
  defaultPositions: Record<number, { x: number; y: number }>
): NodeType[] => {
  const currentPositions = new Map<number, { x: number; y: number; fx: number | null; fy: number | null }>();
  
  existingNodes.forEach(node => {
    if (node.x !== undefined && node.y !== undefined) {
      currentPositions.set(node.id, { 
        x: node.x, 
        y: node.y,
        fx: node.x,
        fy: node.y
      });
    }
  });

  return (data.meas as [string, string][]).map(([basis, phase], index) => {
    const savedPos = currentPositions.get(index);
    const defaultPos = defaultPositions[index];
    
    return {
      id: index,
      basis,
      phase: phase || undefined,
      x: savedPos?.x ?? defaultPos?.x,
      y: savedPos?.y ?? defaultPos?.y,
      fx: savedPos?.fx ?? defaultPos?.x,
      fy: savedPos?.fy ?? defaultPos?.y,
    };
  });
};