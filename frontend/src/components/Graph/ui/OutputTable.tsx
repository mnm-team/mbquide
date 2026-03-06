import { RefObject, FC } from 'react';
import { OutputAdjustment } from '../types';

type OutputTableProps = {
  nodeId: number;
  position: { x: number; y: number };
  adjustment: OutputAdjustment;
  svgRef: RefObject<SVGSVGElement | null>;
};

export const OutputTable: FC<OutputTableProps> = ({
  nodeId,
  position,
  adjustment,
  svgRef,
}) => {
  const svgElement = svgRef.current;
  if (!svgElement) return null;
  
  const rect = svgElement.getBoundingClientRect();
  const svgWidth = 1920;
  const svgHeight = 900;
  const scaleX = rect.width / svgWidth;
  const scaleY = rect.height / svgHeight;
  
  const screenX = position.x * scaleX + rect.left;
  const screenY = position.y * scaleY + rect.top;
  
  return (
    <div
      key={nodeId}
      style={{
        position: 'fixed',
        left: screenX + 30,
        top: screenY - 40,
        backgroundColor: 'white',
        border: '1px solid #ccc',
        borderRadius: '4px',
        padding: '4px',
        fontSize: '12px',
        boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
        zIndex: 100,
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