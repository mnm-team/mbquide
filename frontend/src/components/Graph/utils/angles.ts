// Angle helpers shared by the phase label rendering and the YZ-unfusion angle handle.
// Mirrors the backend's radiansToString()/parseAngle() (backend/src/utils.cpp) so that a
// phase string round-trips between frontend and backend without drifting.

const PI = Math.PI;
const PI_OVER_8 = PI / 8;

export const normalizeRadians = (radians: number): number => {
  let a = radians % (2 * PI);
  if (a < 0) a += 2 * PI;
  return a;
};

const PRETTY_ANGLE_TO_RADIANS: Record<string, number> = {
  '': 0,
  'π/8': PI / 8,
  'π/4': PI / 4,
  '3π/8': 3 * PI / 8,
  'π/2': PI / 2,
  '5π/8': 5 * PI / 8,
  '3π/4': 3 * PI / 4,
  '7π/8': 7 * PI / 8,
  'π': PI,
  '9π/8': 9 * PI / 8,
  '5π/4': 5 * PI / 4,
  '11π/8': 11 * PI / 8,
  '3π/2': 3 * PI / 2,
  '13π/8': 13 * PI / 8,
  '7π/4': 7 * PI / 4,
  '15π/8': 15 * PI / 8,
  '2π': 0,
};

// Parses a node's `phase` field, which is either a pretty backend string (e.g. "3π/8") or a
// plain decimal string (set directly by frontend interactions before the next backend fetch).
export const parsePhaseString = (phase?: string): number => {
  if (phase === undefined || phase === null) return 0;
  if (phase in PRETTY_ANGLE_TO_RADIANS) return PRETTY_ANGLE_TO_RADIANS[phase];
  const parsed = parseFloat(phase);
  return Number.isFinite(parsed) ? normalizeRadians(parsed) : 0;
};

const EIGHTH_PI_LABELS = [
  '0', 'π/8', 'π/4', '3π/8', 'π/2', '5π/8', '3π/4', '7π/8',
  'π', '9π/8', '5π/4', '11π/8', '3π/2', '13π/8', '7π/4', '15π/8',
];

// Rounds to the nearest step of pi/8 and returns the step count (0-15).
export const angleToEighthPiStep = (radians: number): number =>
  Math.round(normalizeRadians(radians) / PI_OVER_8) % 16;

export const snapToEighthPi = (radians: number): number =>
  angleToEighthPiStep(radians) * PI_OVER_8;

export const formatEighthPi = (radians: number): string =>
  EIGHTH_PI_LABELS[angleToEighthPiStep(radians)];
