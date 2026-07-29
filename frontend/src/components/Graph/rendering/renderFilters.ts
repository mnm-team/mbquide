import * as d3 from 'd3';
import { GLOW_INTENSITY, HALO } from '../utils/constants';
import { GLOW_COLORS } from '../utils/colors';

export const createGlowFilter = (
  defs: d3.Selection<SVGDefsElement, unknown, null, undefined>,
  id: string,
  color: string,
  intensity: number
) => {
  const filter = defs.append('filter')
    .attr('id', id)
    .attr('x', '-50%')
    .attr('y', '-50%')
    .attr('width', '200%')
    .attr('height', '200%');

  filter.append('feGaussianBlur')
    .attr('stdDeviation', intensity)
    .attr('result', 'coloredBlur');

  filter.append('feFlood')
    .attr('flood-color', color)
    .attr('flood-opacity', '0.8')
    .attr('result', 'glowColor');

  filter.append('feComposite')
    .attr('in', 'glowColor')
    .attr('in2', 'coloredBlur')
    .attr('operator', 'in')
    .attr('result', 'coloredGlow');

  const feMerge = filter.append('feMerge');
  feMerge.append('feMergeNode').attr('in', 'coloredGlow');
  feMerge.append('feMergeNode').attr('in', 'SourceGraphic');
};

export const createBlurFilter = (
  defs: d3.Selection<SVGDefsElement, unknown, null, undefined>,
  id: string,
  stdDeviation: number
) => {
  defs.append('filter')
    .attr('id', id)
    .attr('x', '-100%')
    .attr('y', '-100%')
    .attr('width', '300%')
    .attr('height', '300%')
    .append('feGaussianBlur')
    .attr('stdDeviation', stdDeviation);
};

export const setupAllFilters = (
  defs: d3.Selection<SVGDefsElement, unknown, null, undefined>
) => {
  createGlowFilter(defs, 'selectedGlow', GLOW_COLORS.SELECTED, GLOW_INTENSITY.SELECTED);
  createBlurFilter(defs, 'haloBlur', HALO.BLUR);
};
