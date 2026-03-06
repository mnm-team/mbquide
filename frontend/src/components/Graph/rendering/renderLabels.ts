import * as d3 from 'd3';
import { NodeType } from '../types';

export const renderBasisLabels = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  nodes: NodeType[],
  colorMap: (d: NodeType) => string
) => {
  return svg
    .selectAll(".label-t")
    .data(nodes)
    .join("text")
    .attr("fill", (d) => colorMap(d))
    .attr("font-size", "13px")
    .attr("font-weight", "bold")
    .attr("pointer-events", "none")
    .attr("text-anchor", "middle")
    .attr("dy", "5px")
    .text((d) => d.basis !== "INPUT" && d.basis !== "OUTPUT" ? d.basis : "");
};

export const renderPhaseLabels = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  nodes: NodeType[]
) => {
  return svg
    .selectAll(".label-phase")
    .data(nodes)
    .join("text")
    .attr("fill", "#000")
    .attr("font-size", "13px")
    .attr("text-anchor", "middle")
    .attr("dy", "30px")
    .text((d) => d.phase || "");
};