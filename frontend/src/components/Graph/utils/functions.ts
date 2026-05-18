import * as d3 from 'd3';
import { NodeType } from '../types';

export function findClosestNode(
  svg: d3.Selection<SVGGElement, unknown, null, undefined>,
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


export function getBoundingCenter(nodes: NodeType[]) {
  const xs = nodes
    .map(n => n.x)
    .filter((x): x is number => x !== undefined);

  const ys = nodes
    .map(n => n.y)
    .filter((y): y is number => y !== undefined);

  if (xs.length === 0 || ys.length === 0) {
    return { x: 0, y: 0 };
  }

  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);

  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);

  return {
    x: (minX + maxX) / 2,
    y: (minY + maxY) / 2,
  };
}