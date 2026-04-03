import { NodeType, Edge } from '../types';

export const useGraphValidation = (
  allNodes: NodeType[],
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
    return areNodesZDeletable(selectedNodes);
  };
  
  const areNodesZDeletable = (ns: NodeType[]): boolean => {
    if (ns.length === 0) return false;
    if (containsOutput()) return false;

    return ns.every(
      (n) =>
        (n.basis === "Z" || n.basis === "XZ" || n.basis === "YZ") &&
        (!n.phase || ["", "\u03c0", "2\u03c0"].includes(n.phase))
    );
  };

  const canSimplify = (): boolean => {
    const normalizeRadians = (angle: number) => {
      let a = angle % (2 * Math.PI);
      if (a < 0) a += 2 * Math.PI;
      return a;
    };

    const fAlmostEqual = (a: number, b: number, eps = 1e-9) => Math.abs(a - b) < eps;

    const planarBases = new Set(['XY', 'XZ', 'YZ']);
    const zBases = new Set(['Z', 'XZ', 'YZ']);

    for (const node of allNodes) {
      const id = node.id;
      const basis = node.basis as string;
      const angle = normalizeRadians(parseFloat(node.phase ?? '0'));
      const isOut = outputs.includes(id);
      const isIn = inputs.includes(id);

      // 1. Relabelable: planar basis, quarter-angle phase, not output
      if (planarBases.has(basis) && !isOut) {
        const isQuarterAngle = fAlmostEqual(angle % (Math.PI / 2), 0);
        if (isQuarterAngle) return true;
      }

      // 2. LC-able: Y node, not input, not output
      if (basis === 'Y' && !isOut && !isIn) return true;

      // 3. Pivot-able: X node, not input, has a non-input neighbor
      if (basis === 'X' && !isIn) {
        const neighbors = edges
          .filter(e => e.source === id || e.target === id)
          .map(e => e.source === id ? e.target : e.source);
        if (neighbors.some(nb => !inputs.includes(nb))) return true;
      }

      // 4. Z-deletable: Z basis, angle is 0 or π, not output
      if (zBases.has(basis) && !isOut) {
        const isValidAngle = fAlmostEqual(angle, 0) || fAlmostEqual(angle, Math.PI);
        if (areNodesZDeletable([node])) return true;
      }

      // 5. Mergeable YZ pair: two non-output YZ nodes, not neighbors, with identical neighbor sets
      const yzNodes = allNodes.filter(n => n.basis === 'YZ' && !outputs.includes(n.id));
      for (let i = 0; i < yzNodes.length; i++) {
        for (let j = i + 1; j < yzNodes.length; j++) {
          const u = yzNodes[i].id;
          const v = yzNodes[j].id;

          const areNeighbors = edges.some(
            e => (e.source === u && e.target === v) || (e.source === v && e.target === u)
          );
          if (areNeighbors) continue;

          const neighborsOf = (id: number) =>
            new Set(
              edges
                .filter(e => e.source === id || e.target === id)
                .map(e => e.source === id ? e.target : e.source)
            );

          const nu = neighborsOf(u);
          const nv = neighborsOf(v);
          if (nu.size === nv.size && [...nu].every(n => nv.has(n))) return true;
        }
      }
    }

    return false;
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
    canSimplify,
    fitForRelabeling,
    areNonPlanar,
  };
};