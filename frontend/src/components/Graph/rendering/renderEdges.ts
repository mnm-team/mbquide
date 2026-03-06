import * as d3 from 'd3';
import { SimEdge } from '../types';
import { EDGE_COLORS } from '../utils/colors';

const EDGE_COLOR_MAPPING: { [key: number]: string } = {
  2: EDGE_COLORS.BLUE,
  3: EDGE_COLORS.GREY,
};

export const renderEdges = (
  svg: d3.Selection<SVGSVGElement | null, unknown, null, undefined>,
  edges: SimEdge[]
) => {
  return svg
    .append("g")
    .attr("stroke-opacity", 0.7)
    .selectAll("line")
    .data(edges)
    .join("line")
    .attr("stroke-width", 2)
    .attr("stroke", (d) => EDGE_COLOR_MAPPING[d.colorCode] || EDGE_COLORS.BLACK);
};