import type { NodeType, Edge, SimEdge, OutputAdjustment, LayerLine } from '../Graph/types';

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
  flowLayerLines?: LayerLine[] | null;
  centerGraphTrigger?: number;
};

export type { NodeType, Edge, SimEdge, OutputAdjustment, LayerLine };