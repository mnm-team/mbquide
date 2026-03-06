import { useState } from 'react';
import { NodeType, OutputTablePosition } from '../types';

export const useOutputTables = () => {
  const [outputTables, setOutputTables] = useState<Record<number, OutputTablePosition>>({});

  const updateOutputTables = (nodes: NodeType[], outputs: number[]) => {
    const newOutputTables: Record<number, OutputTablePosition> = {};
    nodes.forEach(node => {
      if (outputs.includes(node.id) && node.x !== undefined && node.y !== undefined) {
        newOutputTables[node.id] = { x: node.x, y: node.y };
      }
    });
    setOutputTables(newOutputTables);
  };

  return { outputTables, updateOutputTables };
};