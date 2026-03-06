import { NodeType } from '../types';

export const getFillColor = (d: NodeType): string => {
  switch (d.basis) {
    case "": return "none";
    case "X": return "#4e92ca";
    case "Y": return "#ffff33";
    case "Z": return "#EB474A";
    case "XY": return "#60BB5D";
    case "YZ": return "#FF9933";
    case "XZ": return "#AA64B4";
    default: return "white";
  }
};

export const getFillColorZX = (d: NodeType): string => {
  switch (d.basis) {
    case "": return "none";
    case "X": return "#e41a1c";
    case "Y": return "#666666";
    case "Z": return "#DDFFDD";
    case "XY": return "#666666";
    case "YZ": return "#666666";
    case "XZ": return "#666666";
    default: return "white";
  }
};


export const getLabelColor = (d: NodeType): string => {
  switch (d.basis) {
    case "": return "#000000";

    // darker fills
    case "X": return "#f2f2f2";
    case "Z": return "#f2f2f2";
    case "XY": return "#f2f2f2";
    case "XZ": return "#f2f2f2";
    
    // lighter fills
    case "Y": return "#1f1f1f";
    case "YZ": return "#1f1f1f";

    default: return "#000000";
  }
};

export const getLabelColorZX = (d: NodeType): string => {
  switch (d.basis) {
    case "X": return "#fff";
    case "Z": return "#575757";
    default: return "#000";
  }
};

export const GLOW_COLORS = {
  SELECTED: '#0066ff',
  CORRECTION: '#ff6600',
  ODDCORRECTION: '#4ef56d',
} as const;

export const EDGE_COLORS = {
  BLUE: 'blue',
  BLACK: 'black',
  GREY: 'grey',
} as const;

export const BRUSH_COLORS = {
  FILL: 'rgba(0, 120, 215, 0.3)',
  STROKE: '#0078d7',
} as const;

export const BACKGROUND_COLOR = '#F9F9F9' as const;