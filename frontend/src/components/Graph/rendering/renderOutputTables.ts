import * as d3 from 'd3';
import { NodeType, OutputAdjustment } from '../types';
import { OUTPUT_TABLE } from '../utils/constants';

export const renderOutputTables = (
  group: d3.Selection<SVGGElement, unknown, null, undefined>,
  nodes: NodeType[],
  outputs: number[],
  outputAdjustments: Record<number, OutputAdjustment>,
) => {
  const outputNodes = nodes.filter(
    n => outputs.includes(n.id) && outputAdjustments[n.id]
  );

  const tableGroup = group
    .selectAll<SVGGElement, NodeType>('g.output-table')
    .data(outputNodes, d => d.id)
    .join('g')
    .attr('class', 'output-table')
    .attr('transform', d => `translate(${(d.x ?? 0) + OUTPUT_TABLE.X_OFFSET}, ${(d.y ?? 0) - OUTPUT_TABLE.Y_OFFSET})`);

  // Background rect
  tableGroup.selectAll('rect.bg').data(d => [d]).join('rect')
    .attr('class', 'bg')
    .attr('x', 0).attr('y', 0)
    .attr('width', 90).attr('height', 52)
    .attr('rx', 4)
    .attr('fill', 'white')
    .attr('stroke', '#ccc')
    .attr('filter', 'drop-shadow(0 2px 4px rgba(0,0,0,0.15))');

  // Header row
  const headers = ['In', 'Out', 'Sign'];
  const colX = [8, 30, 60];
  headers.forEach((h, i) => {
    tableGroup.selectAll(`text.th-${i}`).data(d => [d]).join('text')
      .attr('class', `th-${i}`)
      .attr('x', colX[i]).attr('y', 13)
      .attr('font-size', OUTPUT_TABLE.FONT_SIZE).attr('font-weight', 'bold')
      .text(h);
  });

  // X row
  const rowY = [30, 44];
  const rowKeys: Array<'X' | 'Z'> = ['X', 'Z'];

  rowKeys.forEach((key, row) => {
    // Row label
    tableGroup.selectAll(`text.label-${key}`).data(d => [d]).join('text')
      .attr('class', `label-${key}`)
      .attr('x', colX[0]+2).attr('y', rowY[row])
      .attr('font-size', OUTPUT_TABLE.FONT_SIZE)
      .text(key);

    // Out value
    tableGroup.selectAll(`text.out-${key}`).data(d => [outputAdjustments[d.id]]).join('text')
      .attr('class', `out-${key}`)
      .attr('x', colX[1]+7).attr('y', rowY[row])
      .attr('font-size', OUTPUT_TABLE.FONT_SIZE).attr('fill', '#333')
      .text(d => d[key][0]);

    // Sign
    tableGroup.selectAll(`text.sign-${key}`).data(d => [outputAdjustments[d.id]]).join('text')
      .attr('class', `sign-${key}`)
      .attr('x', colX[2]+6).attr('y', rowY[row])
      .attr('font-size', OUTPUT_TABLE.FONT_SIZE)
      .text(d => d[key][1] < 0 ? '−' : '+');
  });

  // Divider line under header
  tableGroup.selectAll('line.divider').data(d => [d]).join('line')
    .attr('class', 'divider')
    .attr('x1', 0).attr('y1', 17).attr('x2', 90).attr('y2', 17)
    .attr('stroke', '#444').attr('stroke-width', 1);

  return tableGroup;
};