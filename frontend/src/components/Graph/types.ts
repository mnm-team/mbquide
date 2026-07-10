
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
  flowDepth?: number;
};

export type Edge = {
  source: number;
  target: number;
  colorCode: number;
};

export type SimEdge = {
  source: NodeType;
  target: NodeType;
  colorCode: number;
};

export type LayerLine = {
  x: number;
  y1: number;
  y2: number;
};

export type OutputAdjustment = {
  X: [string, number];
  Z: [string, number];
};

export type GraphProps = {
  nodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  outputAdjustments?: Record<string, OutputAdjustment>;
  onSelectionChange?: (selected: NodeType[]) => void;
  onNodeDrop?: (node?: NodeType, x?: number, y?: number, isInput?: boolean) => void;
  onNodeDragEnd?: (nodes: NodeType[]) => void;
  onNodeDelete?: (nodes?: NodeType[]) => void;
  onCreateNewEdge?: (edge?: Edge) => void;
  runLocalComplementation?: () => void;
  runRelabelingPlanar?: (basis: string | undefined) => void;
  runRelabeling?: () => void;
  onPhaseSubmit?: (node?: NodeType, angle?: number) => void;
  buildingMode?: boolean
  centerGraphTrigger?: number;
  flowLayerLines?: LayerLine[] | null;
};

export type ContextMenuState = {
  visible: boolean;
  x: number;
  y: number;
  node: NodeType | null;
};

export type OutputTablePosition = {
  x: number;
  y: number;
};