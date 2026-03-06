import React from 'react';
import { OutputAdjustment } from '../types';

type SimulatorOutputTableProps = {
  nodeId: number;
  position: { x: number; y: number };
  panOffset: { x: number; y: number };
  adjustment: OutputAdjustment;
  width: number;
};

export const SimulatorOutputTable: React.FC<SimulatorOutputTableProps> = ({
  nodeId,
  position,
  panOffset,
  adjustment,
  width,
}) => {
  const xOffset = (7 / width) * 10000;
  const yOffset = (7 / width) * 10000 - 70;

  return (
    <div
      key={nodeId}
      style={{
        position: 'fixed',
        // Add panOffset so the table tracks the node as the user pans around
        left: position.x + xOffset + panOffset.x,
        top: position.y - yOffset + panOffset.y,
        backgroundColor: 'white',
        border: '1px solid #ccc',
        borderRadius: '4px',
        padding: '4px',
        fontSize: '12px',
        boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
        zIndex: 0,
        pointerEvents: 'none'
      }}
    >
      <table style={{ borderCollapse: 'collapse', minWidth: '80px' }}>
        <thead>
          <tr>
            <th style={{ border: '1px solid #ddd', padding: '2px 4px', fontSize: '10px' }}>In</th>
            <th style={{ border: '1px solid #ddd', padding: '2px 4px', fontSize: '10px' }}>Out</th>
            <th style={{ border: '1px solid #ddd', padding: '2px 4px', fontSize: '10px' }}>Sign</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>X</td>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>{adjustment.X[0]}</td>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>
              {adjustment.X[1] < 0 ? "-" : "+"}
            </td>
          </tr>
          <tr>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>Z</td>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>{adjustment.Z[0]}</td>
            <td style={{ border: '1px solid #ddd', padding: '2px 4px', textAlign: 'center' }}>
              {adjustment.Z[1] < 0 ? "-" : "+"}
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  );
};