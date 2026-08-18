
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

// Which node of a YZ-unfusion pair an angle applies to: the XY node's own angle (beta), or the
// pendant YZ node's own angle - the other one is then derived via the invariant alpha = xy - yz.
export type UnfusionTarget = 'xy' | 'yz';

export type GraphProps = {
  nodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  outputAdjustments?: Record<string, OutputAdjustment>;
  onSelectionChange?: (selected: NodeType[]) => void;
  onNodeDrop?: (node?: NodeType, x?: number, y?: number, isInput?: boolean) => void;
  onNodeDragStart?: () => void;
  onNodeDragEnd?: (nodes: NodeType[]) => void;
  onNodeDelete?: (nodes?: NodeType[]) => void;
  onCreateNewEdge?: (edge?: Edge) => void;
  runLocalComplementation?: () => void;
  runRelabelingPlanar?: (basis: string | undefined) => void;
  runRelabeling?: () => void;
  onPhaseSubmit?: (node?: NodeType, angle?: number) => void;
  runYZUnfusion?: (node: NodeType) => void;
  onYZAngleChange?: (xyNode: NodeType, yzNode: NodeType, angle: number, target: UnfusionTarget) => void;
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