import * as d3 from 'd3';
import { NodeType, UnfusionTarget } from '../types';
import { UnfusionPair, getTravelSegment, updateUnfusionHandles } from '../rendering/renderUnfusionHandles';
import { normalizeRadians, parsePhaseString, snapToEighthPi, formatEighthPi } from '../utils/angles';

// Dragging slides the handle along the (inset) XY <-> YZ travel segment, snapping to pi/8 steps
// of beta (the new shared angle from YZUnfusion). Sitting right next to the XY node means the
// full turn is "here" (beta = 2pi, i.e. normalized to 0), sliding towards the YZ node winds it
// back down; see handleFraction() in renderUnfusionHandles.ts for the same mapping used to
// position the handle at rest. alphaOriginal = beta_old - yz_old is invariant across drags
// (it's the node's original, pre-unfusion angle), so each tick recomputes the YZ node's angle
// as beta_new - alphaOriginal to keep the pair state-preserving relative to that original angle.
export const createUnfusionAngleDrag = (
  panGroup: d3.Selection<SVGGElement, unknown, null, undefined>,
  groups: d3.Selection<SVGGElement, UnfusionPair, SVGGElement, unknown>,
  onCommit?: (xyNode: NodeType, yzNode: NodeType, angle: number, target: UnfusionTarget) => void,
  labelsPhase?: d3.Selection<d3.BaseType, NodeType, SVGGElement, unknown>
) => {
  let alphaOriginal = 0;
  let moved = false;

  // Attached to the knob bar only (see useGraphSimulation.ts), so the wire/ticks stay
  // purely decorative and only grabbing the bead itself starts a drag.
  return d3
    .drag<SVGLineElement, UnfusionPair>()
    .filter((event) => event.button === 0)
    .on('start', function (_event, d) {
      const betaOld = normalizeRadians(parsePhaseString(d.xy.phase));
      const yzOld = normalizeRadians(parsePhaseString(d.yz.phase));
      alphaOriginal = normalizeRadians(betaOld - yzOld);
      moved = false;

      d3.select(this.parentNode as Element).raise();
      d3.select(this).style('cursor', 'grabbing');
    })
    .on('drag', (event, d) => {
      moved = true;

      const [px, py] = d3.pointer(event, panGroup.node());
      const seg = getTravelSegment(d);
      const dx = seg.x2 - seg.x1;
      const dy = seg.y2 - seg.y1;
      const len2 = dx * dx + dy * dy;

      let t = len2 === 0 ? 0 : ((px - seg.x1) * dx + (py - seg.y1) * dy) / len2;
      t = Math.max(0, Math.min(1, t));

      const beta = snapToEighthPi((1 - t) * 2 * Math.PI);

      d.xy.phase = beta.toString();
      d.yz.phase = normalizeRadians(beta - alphaOriginal).toString();

      updateUnfusionHandles(groups);

      // Show the pi-fraction form during the drag too (matching what the backend will render
      // once committed - including a blank label for angle 0), rather than the raw decimal
      // that .phase is stored as mid-drag.
      labelsPhase
        ?.filter((n) => n.id === d.xy.id || n.id === d.yz.id)
        .text((n) => {
          const label = formatEighthPi(normalizeRadians(parsePhaseString(n.phase)));
          return label === '0' ? '' : label;
        });
    })
    .on('end', function (_event, d) {
      d3.select(this).style('cursor', 'grab');

      // A plain click (no movement) fires a full drag start/end with nothing to commit.
      // Skip it: committing here would re-render the whole graph and tear down the DOM
      // mid double-click, which is exactly what made double-clicking the handle unreliable.
      if (!moved) return;

      if (onCommit) {
        // Dragging always moves the XY node's own angle (beta); the pendant's angle is
        // derived from it (see the drag handler above).
        onCommit(d.xy, d.yz, normalizeRadians(parsePhaseString(d.xy.phase)), 'xy');
      }
    });
};
