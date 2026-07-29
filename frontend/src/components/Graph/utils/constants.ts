export const SVG_DIMENSIONS = {
  WIDTH: 1920,
  HEIGHT: 900,
} as const;

export const EXAMPLE_CONFIG = {
  X_OFFSET: 1850,
  Y_OFFSET: 200,
  Y_DISTANCE: 70,
} as const;

export const NODE_SIZES = {
  CIRCLE_RADIUS: 16,
  CIRCLE_OUTER_RADIUS: 20,
  RECT_SIZE: 24,
  RECT_OUTER_SIZE: 28,
} as const;

export const GLOW_INTENSITY = {
  SELECTED: 7,
} as const;

export const HALO = {
  GAP: 6, // Gap between a node own outer edge and innermost halo ring
  RING_GAP: 2,  // Gap between the correction-set ring and the odd-neighborhood ring
  
  STROKE_WIDTH: 2.2,
  BLUR: 0.6,
} as const;

export const OUTPUT_TABLE = {
  X_OFFSET: 30,
  Y_OFFSET: -30,
  FONT_SIZE: 13,
} as const;