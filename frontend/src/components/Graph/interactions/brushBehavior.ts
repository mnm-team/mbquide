import * as d3 from 'd3';
import { NodeType } from '../types';

export const createBrushBehavior = (
  width: number,
  height: number,
  nodes: NodeType[],
  setSelectedNodes: (nodes: NodeType[]) => void,
  onSelectionChange?: (selected: NodeType[]) => void,
  getPanOffset?: () => { x: number; y: number },  // ← add this
) => {
  const getOffset = getPanOffset ?? (() => ({ x: 0, y: 0 }));

  return d3.brush()
    .extent([[0, 0], [width, height]])
    .on("start", () => {
      setSelectedNodes([]);
    })
    .on("brush", (event) => {
      if (!event.selection) return;
      const [[x0, y0], [x1, y1]] = event.selection;
      const { x: px, y: py } = getOffset();
      const selected = nodes.filter((d) =>
        x0 <= d.x! + px && d.x! + px < x1 &&
        y0 <= d.y! + py && d.y! + py < y1
      );
      setSelectedNodes(selected);
    })
    .on("end", (event) => {
      if (!event.selection) return;
      const brushLayer = d3.select(event.sourceEvent.target.parentNode);
      brushLayer.call(d3.brush().move as any, null);
      const [[x0, y0], [x1, y1]] = event.selection;
      const { x: px, y: py } = getOffset();
      const selected = nodes.filter((d) =>
        x0 <= d.x! + px && d.x! + px < x1 &&
        y0 <= d.y! + py && d.y! + py < y1
      );
      setSelectedNodes(selected);
      if (onSelectionChange) onSelectionChange(selected);
    });
};