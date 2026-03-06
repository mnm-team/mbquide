import * as d3 from 'd3';
import { NodeType } from '../types';
import { EXAMPLE_CONFIG, NODE_SIZES } from '../utils/constants';
import { getFillColor, getLabelColor, BACKGROUND_COLOR } from '../utils/colors';

export const createExampleNodes = (): NodeType[] => {
  const { X_OFFSET, Y_OFFSET, Y_DISTANCE } = EXAMPLE_CONFIG;
  
  return [
    { id: 10001, basis: "X", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET, fy: Y_OFFSET },
    { id: 10002, basis: "Y", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 1, fy: Y_OFFSET + Y_DISTANCE * 1 },
    { id: 10003, basis: "Z", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 2, fy: Y_OFFSET + Y_DISTANCE * 2 },
    { id: 10004, basis: "XY", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 3, fy: Y_OFFSET + Y_DISTANCE * 3 },
    { id: 10005, basis: "YZ", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 4, fy: Y_OFFSET + Y_DISTANCE * 4 },
    { id: 10006, basis: "XZ", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 5, fy: Y_OFFSET + Y_DISTANCE * 5 },
    { id: 10008, basis: "INPUT", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 6, fy: Y_OFFSET + Y_DISTANCE * 6 },
    { id: 10009, basis: "OUTPUT", x: X_OFFSET, fx: X_OFFSET, y: Y_OFFSET + Y_DISTANCE * 7, fy: Y_OFFSET + Y_DISTANCE * 7 },
  ];
};

export const renderExampleNodeShapes = (
  group: d3.Selection<SVGGElement, NodeType, SVGGElement, unknown>
) => {
  group.each(function (d) {
    const g = d3.select(this);

    if (d.id === 10008) {
      g.append("rect")
        .attr("x", -NODE_SIZES.RECT_SIZE / 2)
        .attr("y", -NODE_SIZES.RECT_SIZE / 2)
        .attr("width", NODE_SIZES.RECT_SIZE)
        .attr("height", NODE_SIZES.RECT_SIZE)
        .attr("fill", BACKGROUND_COLOR)
        .attr("stroke", "black")
        .attr("stroke-width", 2);
    } else if (d.id === 10009) {
      g.append("circle")
        .attr("r", NODE_SIZES.CIRCLE_RADIUS)
        .attr("fill", BACKGROUND_COLOR)
        .attr("stroke", "black")
        .attr("stroke-width", 2);
      g.append("circle")
        .attr("r", NODE_SIZES.CIRCLE_OUTER_RADIUS)
        .attr("fill", "none")
        .attr("stroke", "black")
        .attr("stroke-width", 2);
    } else {
      g.append("circle")
        .attr("r", NODE_SIZES.CIRCLE_RADIUS)
        .attr("fill", getFillColor(d))
        .attr("stroke", "black")
        .attr("stroke-width", 1);
    }
  });
};

export const renderExampleLabels = (
  exampleGroup: d3.Selection<SVGGElement, unknown, null, undefined>,
  exampleNodes: NodeType[]
) => {
  return exampleGroup
    .selectAll(".example-label")
    .data(exampleNodes)
    .join("text")
    .attr("fill", (d) => getLabelColor(d))
    .attr("font-size", "13px")
    .attr("font-weight", "bold")
    .attr("pointer-events", "none")
    .attr("text-anchor", "middle")
    .attr("x", (d) => d.x!)
    .attr("y", (d) => d.y!)
    .attr("dy", "5px")
    .text((d) => d.basis !== "INPUT" && d.basis !== "OUTPUT" ? d.basis : "");
};