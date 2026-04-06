import * as d3 from 'd3';
import { NodeType } from '../types';
import { getFillColor } from '../utils/colors';
import { NODE_SIZES } from '../utils/constants';

declare global {
  interface SVGGElement {
    __dragClone?: d3.Selection<SVGGElement, unknown, null, undefined>;
  }
}

export const createExampleDragBehavior = (
  panGroup: d3.Selection<SVGGElement, unknown, null, undefined>,
  exampleNodes: d3.Selection<SVGGElement, NodeType, SVGGElement, unknown>,
  onNodeDrop?: (node?: NodeType, x?: number, y?: number, isInput?: boolean) => void,
  getPanOffset?: () => { x: number; y: number },  // Needed because example nodes not a part of pan Layer
) => {
  let inputBasis = "";

  return d3.drag<SVGGElement, NodeType, NodeType>()
    .on("start", function(_event, d) {
      const selection = d3.select(this) as d3.Selection<SVGGElement, NodeType, any, any>;
      selection.style("cursor", "grabbing");

      // Clone goes into panGroup so it shares the same coordinate space
      const { x: px, y: py } = getPanOffset?.() ?? { x: 0, y: 0 };
      const clone = panGroup.append("g")
        .attr("class", "dragging-clone")
        .attr("transform", `translate(${d.x! - px},${d.y! - py})`)
        .style("pointer-events", "none");

      if (d.id === 10008) {
        clone.append("rect")
          .attr("x", -NODE_SIZES.RECT_SIZE / 2)
          .attr("y", -NODE_SIZES.RECT_SIZE / 2)
          .attr("width", NODE_SIZES.RECT_SIZE)
          .attr("height", NODE_SIZES.RECT_SIZE)
          .attr("fill", "none")
          .attr("stroke", "black")
          .attr("stroke-width", 2);
      } else if (d.id === 10009) {
        clone.append("circle")
          .attr("r", NODE_SIZES.CIRCLE_RADIUS)
          .attr("fill", "none")
          .attr("stroke", "black")
          .attr("stroke-width", 2);
        clone.append("circle")
          .attr("r", NODE_SIZES.CIRCLE_OUTER_RADIUS)
          .attr("fill", "none")
          .attr("stroke", "black")
          .attr("stroke-width", 2);
      } else {
        clone.append("circle")
          .attr("r", NODE_SIZES.CIRCLE_RADIUS)
          .attr("fill", getFillColor(d))
          .attr("stroke", "black")
          .attr("stroke-width", 1);
      }

      clone.append("text")
        .attr("fill", "#fff")
        .attr("font-size", "13px")
        .attr("font-weight", "bold")
        .attr("text-anchor", "middle")
        .attr("dy", "5px")
        .text(d.basis !== "INPUT" && d.basis !== "OUTPUT" ? d.basis : "");

      selection.node()!.__dragClone = clone;
    })
    .on("drag", function(event, d) {
      const clone = d3.select(this).node()!.__dragClone;
      const { x: px, y: py } = getPanOffset?.() ?? { x: 0, y: 0 };

      if (clone) {
        // event.x/y are in SVG root coordinates; subtract pan offset to get panGroup coords
        clone.attr("transform", `translate(${event.x - px},${event.y - py})`);
      }

      if (d.basis === "INPUT") {
        let hoveredBasis: string | null = null;
        exampleNodes.each(function(n) {
          if (["X", "Y", "XY"].includes(n.basis)) {
            const node = d3.select(this);
            const transform = node.attr("transform");
            const match = /translate\(([^,]+),([^)]+)\)/.exec(transform);
            if (!match) return;
            const x = parseFloat(match[1]);
            const y = parseFloat(match[2]);
            const radius = 30;
            if (
              event.x >= x - radius && event.x <= x + radius &&
              event.y >= y - radius && event.y <= y + radius
            ) {
              hoveredBasis = n.basis;
              clone?.select("rect")?.attr("fill", getFillColor(n));
            }
          }
        });
        if (hoveredBasis) {
          inputBasis = hoveredBasis;
        }
      }
    })
    .on("end", function(event, d) {
      const selection = d3.select(this) as d3.Selection<SVGGElement, NodeType, any, any>;
      selection.style("cursor", "grab");

      const clone = selection.node()!.__dragClone;
      if (clone) {
        clone.remove();
        selection.node()!.__dragClone = undefined;
      }

      if (onNodeDrop) {
        // Subtract pan offset so the node is placed in graph coordinates
        const { x: px, y: py } = getPanOffset?.() ?? { x: 0, y: 0 };
        const graphX = event.x - px;
        const graphY = event.y - py;

        if (d.basis === "INPUT") {
          if (inputBasis === "INPUT") return;
          d.basis = inputBasis;
          onNodeDrop(d, graphX, graphY, true);
        } else {
          onNodeDrop(d, graphX, graphY);
        }
      }
    });
};