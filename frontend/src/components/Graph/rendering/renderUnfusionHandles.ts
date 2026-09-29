import * as d3 from 'd3';
import { NodeType, SimEdge } from '../types';
import { normalizeRadians, parsePhaseString } from '../utils/angles';
import { NODE_SIZES } from '../utils/constants';
import {
  TOOLTIP_BG,
  TOOLTIP_TEXT_COLOR,
  TOOLTIP_RADIUS,
  TOOLTIP_PADDING_X,
  TOOLTIP_GAP,
  UNFUSION_TOOLTIP_FONT_SIZE,
  getTooltipHeight,
} from '../../Tooltip';

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

// Hover tooltip on the knob, styled after ActionButton's `sublabelAsTooltip` pill (see
// ../../Tooltip.tsx for the shared style constants).
const UNFUSION_TOOLTIP_TEXT = 'Double-click to enter an exact angle';

// The bead's travel path gets a little extra breathing room beyond the node borders.
export const getTravelSegment = (pair: UnfusionPair) => insetSegment(pair, NODE_SIZES.CIRCLE_RADIUS + 4);

// The handle slides the full length of the (inset) edge as beta sweeps 2pi -> 0: sitting right
// next to the XY node represents the full turn (beta = 2pi, i.e. "all of it is here"), sliding
// towards the YZ node winds it back down to 0.
export const handleFraction = (pair: UnfusionPair): number =>
  1 - normalizeRadians(parsePhaseString(pair.xy.phase)) / (2 * Math.PI);

// How long the pointer must rest on the knob before the tooltip appears - long enough that it
// doesn't flash in while just passing over the wire or grabbing the knob to start a drag.
const TOOLTIP_HOVER_DELAY_MS = 1500;

// Pending "show the tooltip" timers, keyed by the knob DOM node so a mouseleave (or a drag
// start, see unfusionAngleDrag.ts) can cancel one before it fires.
const hoverTimers = new WeakMap<SVGLineElement, ReturnType<typeof setTimeout>>();

const clearHoverTimer = (node: SVGLineElement) => {
  const existing = hoverTimers.get(node);
  if (existing !== undefined) {
    clearTimeout(existing);
    hoverTimers.delete(node);
  }
};

const fadeTooltip = (node: SVGLineElement, opacity: 0 | 1, durationMs: number) => {
  d3.select(node.parentNode as SVGGElement)
    .select('g.unfusion-tooltip')
    .transition('unfusion-tooltip-fade')
    .duration(durationMs)
    .style('opacity', opacity);
};

// Hides the tooltip (if shown) and cancels any pending reveal. Exported so the drag behavior in
// unfusionAngleDrag.ts - which shares these same knob nodes - can suppress the tooltip for the
// duration of a drag instead of letting it pop up while the bead is being moved.
export const cancelUnfusionTooltipHover = (node: SVGLineElement, tooltipTransitionMs: number) => {
  clearHoverTimer(node);
  fadeTooltip(node, 0, tooltipTransitionMs);
};

// (Re)starts the hover-delay countdown to reveal the tooltip. Also used by unfusionAngleDrag.ts
// on drag end, so resting on the knob after a drag still reveals the tooltip after the delay.
export const scheduleUnfusionTooltipHover = (node: SVGLineElement, tooltipTransitionMs: number) => {
  clearHoverTimer(node);
  const timer = setTimeout(() => {
    hoverTimers.delete(node);
    fadeTooltip(node, 1, tooltipTransitionMs);
  }, TOOLTIP_HOVER_DELAY_MS);
  hoverTimers.set(node, timer);
};

export const renderUnfusionHandles = (
  panGroup: d3.Selection<SVGGElement, unknown, null, undefined>,
  pairs: UnfusionPair[],
  // Just the fade-in/out transition itself - the "shouldn't flash in immediately" behavior now
  // comes from TOOLTIP_HOVER_DELAY_MS above, so this only needs to be a quick, ordinary fade.
  tooltipTransitionMs = 200
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

  const knobs = groups
    .append('line')
    .attr('class', 'unfusion-knob')
    .attr('stroke', '#FF9933')
    .attr('stroke-width', 6)
    .attr('stroke-linecap', 'round')
    .style('cursor', 'grab')
    .style('pointer-events', 'all');

  // Hover tooltip for the knob, styled like ActionButton's `sublabelAsTooltip` (dark rounded
  // pill, white text, fades in above the target) but drawn in SVG since the knob lives on the
  // d3-rendered canvas rather than in React.
  const tooltips = groups
    .append('g')
    .attr('class', 'unfusion-tooltip')
    .style('opacity', 0)
    .style('pointer-events', 'none');

  tooltips
    .append('rect')
    .attr('class', 'unfusion-tooltip-bg')
    .attr('rx', TOOLTIP_RADIUS)
    .attr('ry', TOOLTIP_RADIUS)
    .attr('fill', TOOLTIP_BG);

  tooltips
    .append('text')
    .attr('class', 'unfusion-tooltip-text')
    .attr('fill', TOOLTIP_TEXT_COLOR)
    .attr('font-size', UNFUSION_TOOLTIP_FONT_SIZE)
    .attr('font-family', 'inherit')
    .attr('text-anchor', 'middle')
    .attr('dominant-baseline', 'middle')
    .text(UNFUSION_TOOLTIP_TEXT);

  // Size the pill to the rendered text (fixed text, so this only needs to run once per handle).
  const tooltipHeight = getTooltipHeight(UNFUSION_TOOLTIP_FONT_SIZE);
  tooltips.each(function () {
    const g = d3.select(this);
    const textNode = g.select<SVGTextElement>('text.unfusion-tooltip-text').node();
    const textWidth = textNode ? textNode.getComputedTextLength() : 0;
    g.select('rect.unfusion-tooltip-bg')
      .attr('width', textWidth + TOOLTIP_PADDING_X * 2)
      .attr('height', tooltipHeight)
      .attr('x', -(textWidth / 2 + TOOLTIP_PADDING_X))
      .attr('y', -tooltipHeight / 2);
  });

  knobs
    .on('mouseenter', function () {
      scheduleUnfusionTooltipHover(this as SVGLineElement, tooltipTransitionMs);
    })
    .on('mouseleave', function () {
      cancelUnfusionTooltipHover(this as SVGLineElement, tooltipTransitionMs);
    });

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

    // Tooltip pill sits directly above the bead, mirroring the `bottom-full mb-2` placement
    // used for ActionButton's tooltip.
    g.select('g.unfusion-tooltip')
      .attr('transform', `translate(${hx}, ${hy - BAR_HALF_LENGTH - TOOLTIP_GAP})`);
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
