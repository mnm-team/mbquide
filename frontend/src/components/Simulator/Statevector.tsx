import React, { useState } from 'react';
import { Checkbox } from '../Buttons';

interface StatevectorComponentProps {
  statevector: number[][];
  ids?: number[]
}

const Statevector: React.FC<StatevectorComponentProps> = ({ statevector, ids }) => {
  const vectors: number[][] = statevector;
  const [showOnlyNonZero, setShowOnlyNonZero] = useState(true);
  
  const getStateInfo = (vector: number[]) => {
    const real = vector[0];
    const imag = vector[1];
    
    // Probability is |amplitude|² = real² + imag²
    const probability = real * real + imag * imag;
    
    const phase = Math.atan2(imag, real) * (180 / Math.PI);  // Convert to degrees
    const normalizedPhase = phase < 0 ? phase + 360 : (phase === 360 ? 0 : phase );  // Normalize to 306°
    
    return { probability, phase: normalizedPhase };
  };
  
  const phaseToColor = (phase: number) => {
    return `hsl(${phase}, 70%, 50%)`;
  };
  
  return (
    <div className="bg-white rounded-lg shadow-md p-4 max-w-2xl h-full flex flex-col">
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2 mb-1">
          <div className="w-1.5 h-1.5 rounded-full bg-blue-500" />
          <span className="text-sm text-blue-500 tracking-widest uppercase font-semibold">
            Statevector
          </span>
        </div>
        <Checkbox
          label="Show Only Non-Zero"
          checked={showOnlyNonZero}
          onChange={setShowOnlyNonZero}
        />
      </div>
      <span className="text-xs font-semibold text-gray-600 w-80 mb-2">&nbsp;{ ids?.join(" ") } &emsp;← Vertex IDs </span>
      <div className="space-y-2 overflow-y-auto flex-1 pr-2">
        {vectors.map((vector, idx) => {
          const { probability, phase } = getStateInfo(vector);
          const barColor = phaseToColor(phase);
          
          // Skip states with zero probability if filter is enabled
          if (showOnlyNonZero && probability.toFixed(2) === "0.00") return null;
          
          return (
            <div key={idx} className="flex items-center gap-2">
              <span className="text-xs font-semibold text-gray-600 w-100">|{idx.toString(2).padStart(Math.log2(vectors.length), '0').split('').join(' ')}⟩:</span>
              
              {/* Probability bar with phase-based color */}
              <div className="flex items-center gap-2">
                <div className="w-32 h-5 bg-gray-200 rounded-full overflow-hidden relative">
                  <div
                    className="h-full transition-all duration-300 rounded-full"
                    style={{
                      width: `${probability * 100}%`,
                      backgroundColor: barColor
                    }}
                  />
                  <span className="absolute inset-0 flex items-center justify-center text-xs font-semibold text-gray-700 whitespace-nowrap">
                    P = {probability.toFixed(2)}
                  </span>
                </div>
                <span className="text-xs text-gray-400 w-10">
                  φ: {phase.toFixed(0)}°
                </span>
              </div>
              
              <span className="px-2 py-0.5 bg-gray-100 text-gray-700 rounded font-mono text-xs whitespace-nowrap w-25">
                {vector[0].toFixed(2)} + {vector[1].toFixed(2)}i
              </span>
            </div>
          );
        })}
      </div>
      
      {/* Phase color legend */}
      <div className="mt-6 pt-4 border-t border-gray-200">
        <p className="text-xs text-gray-600 mb-2 font-semibold">Phase Color Map:</p>
        <div className="h-4 rounded-full" style={{
          background: 'linear-gradient(to right, hsl(0, 70%, 50%), hsl(60, 70%, 50%), hsl(120, 70%, 50%), hsl(180, 70%, 50%), hsl(240, 70%, 50%), hsl(300, 70%, 50%), hsl(360, 70%, 50%))'
        }} />
        <div className="flex justify-between text-xs text-gray-500 mt-1">
          <span>0°</span>
          <span>90°</span>
          <span>180°</span>
          <span>270°</span>
          <span>360°</span>
        </div>
      </div>
    </div>
  );
};

export default Statevector;