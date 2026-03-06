import * as d3 from 'd3';
import { NodeType } from '../types';

export function findClosestNode(
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  x: number,
  y: number,
  maxDistance: number = Infinity
): NodeType | null {
  let closest: NodeType | null = null;

  svg.selectAll<SVGGElement, NodeType>("g.node").each(function(d) {
    const cx = d.x!;
    const cy = d.y!;
    const dist = Math.hypot(cx - x, cy - y);

    if (dist <= maxDistance) {
      closest = d;
    }
  });

  return closest;
}
