import { useState } from 'react';
import { SimulatorGraphProps, NodeType } from './types';
import { useSimulatorRendering } from './hooks/useSimulatorRendering';
import { SimulatorOutputTable } from './ui/OutputTable';

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
  height
}: SimulatorGraphProps) {
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);

  const { svgRef, panOffset } = useSimulatorRendering({
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
  });

  return (
    <div className='w-full h-full'>
      <svg
        ref={svgRef}
        viewBox={`0 0 ${width} ${height}`}
        width={width}
        height={height}
        preserveAspectRatio="xMidYMid meet"
      />
    </div>
  );
}