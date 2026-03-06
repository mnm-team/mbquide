import * as d3 from 'd3';
import { GLOW_INTENSITY } from '../utils/constants';
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

export const createCombinedGlowFilter = (
  defs: d3.Selection<SVGDefsElement, unknown, null, undefined>
) => {
  const combinedFilter = defs.append('filter')
    .attr('id', 'selectedCorrectionGlow')
    .attr('x', '-100%')
    .attr('y', '-100%')
    .attr('width', '300%')
    .attr('height', '300%');

  // Blue inner glow
  combinedFilter.append('feGaussianBlur')
    .attr('in', 'SourceAlpha')
    .attr('stdDeviation', GLOW_INTENSITY.SELECTED_INNER)
    .attr('result', 'blueBlur');

  combinedFilter.append('feFlood')
    .attr('flood-color', GLOW_COLORS.SELECTED)
    .attr('flood-opacity', '1')
    .attr('result', 'blueColor');

  combinedFilter.append('feComposite')
    .attr('in', 'blueColor')
    .attr('in2', 'blueBlur')
    .attr('operator', 'in')
    .attr('result', 'blueGlow');

  // Orange outer glow
  combinedFilter.append('feGaussianBlur')
    .attr('in', 'SourceAlpha')
    .attr('stdDeviation', GLOW_INTENSITY.SELECTED_OUTER)
    .attr('result', 'orangeBlur');

  combinedFilter.append('feFlood')
    .attr('flood-color', GLOW_COLORS.CORRECTION)
    .attr('flood-opacity', '1')
    .attr('result', 'orangeColor');

  combinedFilter.append('feComposite')
    .attr('in', 'orangeColor')
    .attr('in2', 'orangeBlur')
    .attr('operator', 'in')
    .attr('result', 'orangeGlow');

  const merge = combinedFilter.append('feMerge');
  merge.append('feMergeNode').attr('in', 'orangeGlow');
  merge.append('feMergeNode').attr('in', 'blueGlow');
  merge.append('feMergeNode').attr('in', 'SourceGraphic');
};

export const setupAllFilters = (
  defs: d3.Selection<SVGDefsElement, unknown, null, undefined>
) => {
  createGlowFilter(defs, 'selectedGlow', GLOW_COLORS.SELECTED, GLOW_INTENSITY.SELECTED);
  createGlowFilter(defs, 'correctionGlow', GLOW_COLORS.CORRECTION, GLOW_INTENSITY.CORRECTION);
  createGlowFilter(defs, 'oddCorrectionGlow', GLOW_COLORS.ODDCORRECTION, GLOW_INTENSITY.CORRECTION);
  createCombinedGlowFilter(defs);
};