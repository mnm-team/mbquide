import { useState } from 'react';
import { SimulatorGraphProps, NodeType } from './types';
import { useSimulatorRendering } from './hooks/useSimulatorRendering';
import { RecenterHint } from '../Graph/ui/RecenterHint';

export default function MBQC_Simulator({
  nodes: mainNodes,
  edges,
  inputs,
  outputs,
  outputAdjustments = {},
  onSelectionChange,
  measureOperation,
  measured,
  active,
  outcomes,
  readyToMeasure,
  width,
  height,
  flowLayerLines,
  centerGraphTrigger,
}: SimulatorGraphProps) {
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);

  const { svgRef, panOffset, scale } = useSimulatorRendering({
    mainNodes,
    edges,
    inputs,
    outputs,
    measured,
    active,
    outcomes,
    readyToMeasure,
    width,
    height,
    selectedNodes,
    setSelectedNodes,
    onSelectionChange,
    measureOperation,
    outputAdjustments,
    flowLayerLines,
    centerGraphTrigger,
  });

  return (
    <div className='relative w-full h-full'>
      <svg
        ref={svgRef}
        viewBox={`0 0 ${width} ${height}`}
        width={width}
        height={height}
        preserveAspectRatio="xMidYMid meet"
      />

      <RecenterHint nodes={mainNodes} offset={panOffset} scale={scale} width={width} height={height} />
    </div>
  );
}