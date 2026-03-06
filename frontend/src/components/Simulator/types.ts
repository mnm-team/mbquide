import type { NodeType, Edge, SimEdge, OutputAdjustment } from '../Graph/types';

export type SimulatorGraphProps = {
  nodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  outputAdjustments?: Record<string, OutputAdjustment>;
  onSelectionChange?: (selected: NodeType[]) => void;
  measureOperation?: (id: number) => void;
  measured: number[];
  active: number[];
  outcomes: [number, number][];
  readyToMeasure: number[];
  width: number;
  height: number;
};

export type { NodeType, Edge, SimEdge, OutputAdjustment };