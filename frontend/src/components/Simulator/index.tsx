import { useState } from 'react';
import { SimulatorGraphProps, NodeType } from './types';
import { useSimulatorRendering } from './hooks/useSimulatorRendering';
import { SimulatorOutputTable } from './ui/OutputTable';
import { useOutputTables } from '../Graph/hooks/useOutputTables';

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

  const { outputTables, updateOutputTables } = useOutputTables();

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
    updateOutputTables,
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

      {/* Output Tables — shifted by panOffset so they stay glued to their output nodes */}
      {Object.entries(outputTables).map(([nodeId, position]) => {
        const node = mainNodes.find(n => n.id === parseInt(nodeId));
        const adjustment = outputAdjustments[parseInt(nodeId)];
        if (!node || !adjustment) return null;

        return (
          <SimulatorOutputTable
            key={nodeId}
            nodeId={parseInt(nodeId)}
            position={position}
            panOffset={panOffset}
            adjustment={adjustment}
            width={width}
          />
        );
      })}
    </div>
  );
}