import * as d3 from 'd3';
import { NodeType } from '../types';
import { NODE_SIZES } from '../utils/constants';

export const renderNodeShapes = (
  node: d3.Selection<SVGGElement, NodeType, SVGGElement, unknown>,
  inputs: number[],
  outputs: number[],
  colorMap: (d: NodeType) => string,
) => {
  node.each(function (d) {
    const isInput = inputs.includes(d.id);
    const isOutput = outputs.includes(d.id);
    const g = d3.select(this);

    if (isInput) {
      if (isOutput) {
        g.append("rect")
          .attr("x", -NODE_SIZES.RECT_OUTER_SIZE / 2)
          .attr("y", -NODE_SIZES.RECT_OUTER_SIZE / 2)
          .attr("width", NODE_SIZES.RECT_OUTER_SIZE)
          .attr("height", NODE_SIZES.RECT_OUTER_SIZE)
          .attr("fill", "none")
          .attr("stroke", "blue")
          .attr("stroke-width", 2);
      }

      g.append("rect")
        .attr("x", -NODE_SIZES.RECT_SIZE / 2)
        .attr("y", -NODE_SIZES.RECT_SIZE / 2)
        .attr("width", NODE_SIZES.RECT_SIZE)
        .attr("height", NODE_SIZES.RECT_SIZE)
        .attr("fill", colorMap(d))
        .attr("stroke", "black")
        .attr("stroke-width", 2);
    } else {
      if (isOutput) {
        g.append("circle")
          .attr("r", NODE_SIZES.CIRCLE_OUTER_RADIUS)
          .attr("fill", "none")
          .attr("stroke", "black")
          .attr("stroke-width", 2);
      }

      g.append("circle")
        .attr("r", NODE_SIZES.CIRCLE_RADIUS)
        .attr("fill", colorMap(d))
        .attr("stroke", "black")
        .attr("stroke-width", 2);
    }
  });
};