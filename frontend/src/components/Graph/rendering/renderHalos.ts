import * as d3 from 'd3';
import { NodeType } from '../types';
import { NODE_SIZES, HALO } from '../utils/constants';
import { GLOW_COLORS } from '../utils/colors';

const getNodeOuterRadius = (d: NodeType, inputs: number[], outputs: number[]): number => {
  const isInput = inputs.includes(d.id);
  const isOutput = outputs.includes(d.id);

  if (isInput) {
    return (isOutput ? NODE_SIZES.RECT_OUTER_SIZE : NODE_SIZES.RECT_SIZE) / 2;
  }
  return isOutput ? NODE_SIZES.CIRCLE_OUTER_RADIUS : NODE_SIZES.CIRCLE_RADIUS;
};

// Appends two concentric, initially-hidden ring halos per node: an inner ring for
// correction-set membership and an outer ring for odd-neighborhood membership.
// Both can be shown at once, independently of the selection glow filter.
export const renderMembershipHalos = (
  node: d3.Selection<SVGGElement, NodeType, SVGGElement, unknown>,
  inputs: number[],
  outputs: number[],
) => {
  node.each(function (d) {
    const g = d3.select(this);
    const baseRadius = getNodeOuterRadius(d, inputs, outputs);

    g.append('circle')
      .attr('class', 'halo-correction')
      .attr('r', baseRadius + HALO.GAP)
      .attr('fill', 'none')
      .attr('stroke', GLOW_COLORS.CORRECTION)
      .attr('stroke-width', HALO.STROKE_WIDTH)
      .attr('stroke-opacity', 0.85)
      .attr('filter', 'url(#haloBlur)')
      .attr('pointer-events', 'none')
      .style('display', 'none');

    g.append('circle')
      .attr('class', 'halo-odd-correction')
      .attr('r', baseRadius + HALO.GAP + HALO.RING_GAP)
      .attr('fill', 'none')
      .attr('stroke', GLOW_COLORS.ODDCORRECTION)
      .attr('stroke-width', HALO.STROKE_WIDTH)
      .attr('stroke-opacity', 0.85)
      .attr('filter', 'url(#haloBlur)')
      .attr('pointer-events', 'none')
      .style('display', 'none');
  });
};
