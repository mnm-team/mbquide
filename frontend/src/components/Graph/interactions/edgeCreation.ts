import * as d3 from 'd3';
import { NodeType, Edge } from '../types';
import { findClosestNode } from '../utils/functions';

export const createEdgeDragBehavior = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  edgePreviewLayer: d3.Selection<SVGGElement, unknown, null, undefined>,
  addEdge?: (edge?: Edge) => void,
) => {
  let sourceNode: NodeType | null = null;
  let previewLine: d3.Selection<SVGLineElement, unknown, null, undefined> | null = null;

  return d3
    .drag<SVGSVGElement, unknown>()
    .filter(event => event.button === 2)
    .on("start", event => {
      
      const [svgX, svgY] = d3.pointer(event, svg.node());
      const rect = svg.node()!.getBoundingClientRect();
      const clientX = rect.left + svgX;
      const clientY = rect.top + svgY;

      const element = findClosestNode(svg, clientX, clientY, 50);
      if (!element) return;
      
      sourceNode = element;

      previewLine = edgePreviewLayer
        .append("line")
        .attr("stroke", "#888")
        .attr("stroke-width", 2)
        .attr("stroke-dasharray", "4 2")
        .attr("pointer-events", "none")
        .attr("x1", sourceNode.x!)
        .attr("y1", sourceNode.y!)
        .attr("x2", sourceNode.x!)
        .attr("y2", sourceNode.y!);
    })
    .on("drag", event => {
      if (!previewLine || !sourceNode) return;

      const [x, y] = d3.pointer(event, svg.node());
      previewLine.attr("x2", x).attr("y2", y);
    })
    .on("end", event => {
      if (!previewLine || !sourceNode) return;
      
      previewLine.remove();
      previewLine = null;
      
      const [svgX, svgY] = d3.pointer(event, svg.node());
      const rect = svg.node()!.getBoundingClientRect();
      const clientX = rect.left + svgX;
      const clientY = rect.top + svgY;
      
      const element = findClosestNode(svg, clientX, clientY, 50);
      if (!element) {
        sourceNode = null;
        return;
      }
      const targetNode = element;
      
      if (addEdge && targetNode && targetNode !== sourceNode) {
        const edge = {
          source: sourceNode.id,
          target: targetNode.id,
          colorCode: 1,
        };
        addEdge(edge);
      }
    });
};
