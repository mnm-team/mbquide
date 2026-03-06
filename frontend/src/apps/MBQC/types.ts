export type NodeType = {
  id: number;
  basis: string;
  phase?: string;
  correctionSet?: number[];
  oddCorrectionSet?: number[];
  x?: number;
  y?: number;
  fx?: number | null;
  fy?: number | null;
};

export type Edge = {
  source: number;
  target: number;
  colorCode: number;
};

export type OutputAdjustment = {
  X: [string, number];
  Z: [string, number];
};

export const emptyOutputAdjustment: OutputAdjustment = {X: ["X", 1], Z: ["Z", 1]}

export type FlowResult = {
  ok: boolean;
  corrf: Record<number, number[]>;
  oddNcorrf: Record<number, number[]>;
  depths: number[];
};

export type GraphApiResponse = {
  meas: [string, string][];
  edges: [number, number][];
  size: number;
  inputs: number[];
  outputs: number[];
  outAdj: Record<string, OutputAdjustment>;
  flow?: FlowResult;
};

export type UpdateGraphOptions = {
  pos?: [number, number];
  deleted_index?: number;
};

export type GraphState = {
  nodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  adjustments: Record<string, OutputAdjustment>;
};

export type HistoryState = GraphState;