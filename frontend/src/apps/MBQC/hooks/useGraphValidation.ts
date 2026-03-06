import { NodeType, Edge } from '../types';

export const useGraphValidation = (
  selectedNodes: NodeType[],
  edges: Edge[],
  inputs: number[],
  outputs: number[]
) => {
  const isPivotable = (): boolean => {
    if (selectedNodes.length !== 2) return false;
    if (containsInput()) return false;
    const [node1Id, node2Id] = [selectedNodes[0].id, selectedNodes[1].id];

    return edges.some(
      (e) =>
        (e.source === node1Id && e.target === node2Id) ||
        (e.source === node2Id && e.target === node1Id)
    );
  };


  const isLCable = (): boolean => {
    if (selectedNodes.length !== 1) return false;
    if (containsInput()) return false;
    
    return true;
  };

  const isZDeletable = (): boolean => {
    if (selectedNodes.length !== 1) return false;
    if (containsOutput()) return false;

    return selectedNodes.every(
      (n) =>
        (n.basis === "Z" || n.basis === "XZ" || n.basis === "YZ") &&
        (!n.phase || ["", "\u03c0", "2\u03c0"].includes(n.phase))
    );
  };

  const fitForRelabeling = (): boolean => {
    return selectedNodes.every(
      (n) =>
        (n.basis === "XY" || n.basis === "XZ" || n.basis === "YZ") &&
        (!n.phase ||
          ["", "\u03c0", "2\u03c0", "\u03c0/2", "3\u03c0/2"].includes(n.phase))
    );
  };

  const containsInput = (): boolean => {
    return selectedNodes.some((n) => inputs.includes(n.id));
  };

  const containsOutput = (): boolean => {
    return selectedNodes.some((n) => outputs.includes(n.id));
  };

  const areNonPlanar = (): boolean => {
    return selectedNodes.every((n) =>
      ["X", "Y", "Z"].includes(n.basis)
    );
  };

  return {
    isLCable,
    isPivotable,
    isZDeletable,
    fitForRelabeling,
    areNonPlanar,
  };
};