import * as d3 from 'd3';
import { NodeType, SimEdge } from '../types';
import { normalizeRadians, parsePhaseString } from '../utils/angles';
import { NODE_SIZES } from '../utils/constants';

// A "YZ-unfusion" edge: an XY node with a pendant YZ node (degree 1) hanging off it. The
// pendant's angle is fully determined by the XY node's angle (beta) and is thus rendered as a
// single draggable handle rather than two independent phase labels. Detected structurally, so
// it lights up for any matching pair regardless of whether it was created via the YZ-Unfuse
// context menu action or built by hand.
export type UnfusionPair = {
  xy: NodeType;
  yz: NodeType;
};

export const findUnfusionPairs = (edges: SimEdge[]): UnfusionPair[] => {
  const degree = new Map<number, number>();
  edges.forEach((e) => {
    const s = (e.source as NodeType).id;
    const t = (e.target as NodeType).id;
    degree.set(s, (degree.get(s) ?? 0) + 1);
    degree.set(t, (degree.get(t) ?? 0) + 1);
  });

  const qualifies = (xy: NodeType, yz: NodeType) =>
    xy.basis === 'XY' && yz.basis === 'YZ' && (degree.get(yz.id) ?? 0) === 1;

  const pairs: UnfusionPair[] = [];
  edges.forEach((edge) => {
    const a = edge.source as NodeType;
    const b = edge.target as NodeType;

    if (qualifies(a, b)) pairs.push({ xy: a, yz: b });
    else if (qualifies(b, a)) pairs.push({ xy: b, yz: a });
  });

  return pairs;
};

const lerp = (a: number, b: number, t: number) => a + (b - a) * t;

// Shrinks the xy<->yz segment by `margin` at each end, so decorations stay clear of the node
// shapes instead of drawing into them.
const insetSegment = (pair: UnfusionPair, margin: number) => {
  const x1 = pair.xy.x ?? 0;
  const y1 = pair.xy.y ?? 0;
  const x2 = pair.yz.x ?? 0;
  const y2 = pair.yz.y ?? 0;
  const dx = x2 - x1;
  const dy = y2 - y1;
  const len = Math.hypot(dx, dy) || 1;
  const ux = dx / len;
  const uy = dy / len;

  // On very short edges, don't let the margin eat the whole segment.
  const clampedMargin = Math.min(margin, len / 2 - 1);

  return {
    x1: x1 + ux * clampedMargin,
    y1: y1 + uy * clampedMargin,
    x2: x2 - ux * clampedMargin,
    y2: y2 - uy * clampedMargin,
    ux,
    uy,
  };
};

// The decorative wire stops flush with the node borders instead of drawing into the shapes.
const getWireSegment = (pair: UnfusionPair) => insetSegment(pair, NODE_SIZES.CIRCLE_RADIUS);

// Half-length of the perpendicular bar drawn at the bead's position.
const BAR_HALF_LENGTH = 9;

// The bead's travel path gets a little extra breathing room beyond the node borders.
export const getTravelSegment = (pair: UnfusionPair) => insetSegment(pair, NODE_SIZES.CIRCLE_RADIUS + 4);

// The handle slides the full length of the (inset) edge as beta sweeps 2pi -> 0: sitting right
// next to the XY node represents the full turn (beta = 2pi, i.e. "all of it is here"), sliding
// towards the YZ node winds it back down to 0.
export const handleFraction = (pair: UnfusionPair): number =>
  1 - normalizeRadians(parsePhaseString(pair.xy.phase)) / (2 * Math.PI);

export const renderUnfusionHandles = (
  panGroup: d3.Selection<SVGGElement, unknown, null, undefined>,
  pairs: UnfusionPair[]
) => {
  const layer = panGroup.append('g').attr('class', 'unfusion-handles');

  const groups = layer
    .selectAll<SVGGElement, UnfusionPair>('g.unfusion-handle')
    .data(pairs, (d: UnfusionPair) => `${d.xy.id}-${d.yz.id}`)
    .join('g')
    .attr('class', 'unfusion-handle');

  // Decorative dashed overlay on top of the plain edge, marking it as adjustable.
  groups
    .append('line')
    .attr('class', 'unfusion-wire')
    .attr('stroke', '#FF9933')
    .attr('stroke-width', 2.5)
    .attr('stroke-dasharray', '1 5')
    .attr('stroke-linecap', 'round')
    .attr('opacity', 0.6)
    .style('pointer-events', 'none');

  // Tick marks at each of the 16 pi/8 stops.
  groups.each(function () {
    d3.select(this)
      .selectAll('circle.unfusion-tick')
      .data(d3.range(16))
      .join('circle')
      .attr('class', 'unfusion-tick')
      .attr('r', 1.5)
      .attr('fill', '#FF9933')
      .attr('opacity', 0.5)
      .style('pointer-events', 'none');
  });

  // The draggable bead, drawn as a thick bar crossing the wire (a white outline layer plus an
  // orange bar on top). This is the only interactive element on the handle - the wire and
  // ticks are decorative only (pointer-events: none), so dragging or clicking anywhere else
  // on the edge does nothing.
  groups
    .append('line')
    .attr('class', 'unfusion-knob-outline')
    .attr('stroke', '#fff')
    .attr('stroke-width', 8)
    .attr('stroke-linecap', 'round')
    .style('pointer-events', 'none');

  groups
    .append('line')
    .attr('class', 'unfusion-knob')
    .attr('stroke', '#FF9933')
    .attr('stroke-width', 6)
    .attr('stroke-linecap', 'round')
    .style('cursor', 'grab')
    .style('pointer-events', 'all');

  updateUnfusionHandles(groups);

  return groups;
};

// Re-reads position/phase off the bound NodeType objects (mutated in place by the simulation
// tick and by the angle drag), so calling this after either keeps the handle in sync without
// needing a fresh data join.
export const updateUnfusionHandles = (
  groups: d3.Selection<SVGGElement, UnfusionPair, SVGGElement, unknown>
) => {
  groups.each(function (pair) {
    const g = d3.select(this);

    // Decorative wire stops flush with the node borders; the interactive bits (ticks, bar)
    // live on the (slightly wider) travel segment so they stay clear of the node bodies.
    const wireSeg = getWireSegment(pair);
    g.select('line.unfusion-wire')
      .attr('x1', wireSeg.x1).attr('y1', wireSeg.y1)
      .attr('x2', wireSeg.x2).attr('y2', wireSeg.y2);

    const seg = getTravelSegment(pair);

    g.selectAll<SVGCircleElement, number>('circle.unfusion-tick')
      .attr('cx', (step) => lerp(seg.x1, seg.x2, step / 16))
      .attr('cy', (step) => lerp(seg.y1, seg.y2, step / 16));

    const t = handleFraction(pair);
    const hx = lerp(seg.x1, seg.x2, t);
    const hy = lerp(seg.y1, seg.y2, t);

    // The bar is drawn perpendicular to the wire, centered on the handle position.
    const perpX = -seg.uy * BAR_HALF_LENGTH;
    const perpY = seg.ux * BAR_HALF_LENGTH;

    g.selectAll('line.unfusion-knob-outline, line.unfusion-knob')
      .attr('x1', hx - perpX).attr('y1', hy - perpY)
      .attr('x2', hx + perpX).attr('y2', hy + perpY);
  });
};

// A handle is only shown while its XY node or its YZ pendant is selected, so the graph isn't
// cluttered with beads on every unfused edge at once.
export const updateUnfusionHandleVisibility = (
  groups: d3.Selection<SVGGElement, UnfusionPair, SVGGElement, unknown>,
  selectedIds: Set<number>
) => {
  groups.style('display', (d) =>
    selectedIds.has(d.xy.id) || selectedIds.has(d.yz.id) ? null : 'none'
  );
};
