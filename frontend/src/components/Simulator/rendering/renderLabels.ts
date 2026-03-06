import * as d3 from 'd3';
import { NodeType } from '../types';
import { getLabelColor } from '../../Graph/utils/colors';

// Custom basis labels that show outcomes
export const renderBasisLabelsWithOutcomes = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  nodes: NodeType[],
  outcomes: [number, number][]
) => {
  return svg
    .selectAll(".label-t")
    .data(nodes)
    .join("text")
    .attr("fill", (d) => {
      const measured = outcomes.some(([id]) => id === d.id);
      return measured ? "#fff" : getLabelColor(d);
    })
    .attr("font-size", "13px")
    .attr("font-weight", "bold")
    .attr("pointer-events", "none")
    .attr("text-anchor", "middle")
    .attr("dy", "5px")
    .text((d) => {
      // Check if this node has a measurement outcome
      const match = outcomes.find(([id, _]) => id === d.id);
      if (match) {
        return match[1].toString();
      }
      // Otherwise show basis (but not INPUT/OUTPUT)
      return d.basis !== "INPUT" && d.basis !== "OUTPUT" ? d.basis : "";
    });
};

export const renderPhaseLabelsSimulator = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  nodes: NodeType[],
  measured: number[],
) => {
  return svg
    .selectAll(".label-phase")
    .data(nodes)
    .join("text")
    .attr("fill", "#000")
    .attr("font-size", "13px")
    .attr("text-anchor", "middle")
    .attr("dy", "30px")
    .attr("dx", "20px")
    .text(d => {
      const isMeasured = measured.some(id => id === d.id);
      return isMeasured ? "" : (d.phase ?? "");
    });
};

// Simulator-specific: render node IDs
export const renderIdLabels = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  nodes: NodeType[]
) => {
  return svg
    .selectAll(".label-id")
    .data(nodes)
    .join("text")
    .attr("fill", "#333")
    .attr("font-size", "13px")
    .attr("font-weight", "bold")
    .attr("text-anchor", "middle")
    .attr("dy", "-20px")
    .attr("dx", "-20px")
    .text((d) => d.id);
};