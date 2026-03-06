export const createLocalComplementationOperation = (nodeId: number) => ({
  operation: "lc",
  node: nodeId,
});

export const createPivotOperation = (nodeId1: number, nodeId2: number) => ({
  operation: "pivot",
  u: nodeId1,
  v: nodeId2,
});

export const createZInsertionOperation = (nodeIds: number[]) => ({
  operation: "z-insert",
  ids: nodeIds,
});

export const createZDeletionOperation = (nodeId: number) => ({
  operation: "z-delete",
  node: nodeId,
});

export const createRelabelingOperation = (nodeId: number) => ({
  operation: "relabel",
  node: nodeId,
});

export const createRelabelingPlanarOperation = (
  nodeId: number,
  preferredBasis?: string
) => {
  const operation: Record<string, any> = {
    operation: "relabel-planar",
    node: nodeId,
  };

  if (preferredBasis && ["XZ", "XY", "YZ"].includes(preferredBasis)) {
    operation["pref-basis"] = preferredBasis;
  }

  return operation;
};

export const createGetFlowOperation = () => ({
  flow: "pauli",
});

export const createFocusFlowOperation = () => ({
  flow: "focus",
});

export const createTransformToZXOperation = () => ({
  transform: true,
});

export const createSimulateOperation = (input: string = "") => ({
  simulate: true,
  random: true,
  input,
});