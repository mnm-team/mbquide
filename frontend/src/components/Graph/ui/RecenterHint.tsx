import { useEffect, useState } from 'react';
import { NodeType } from '../types';

interface RecenterHintProps {
  nodes: NodeType[];
  offset: { x: number; y: number };
  scale?: number;
  width: number;
  height: number;
}


export function RecenterHint({ nodes, offset, scale = 1, width, height }: RecenterHintProps) {
  const [visible, setVisible] = useState(false);

  useEffect(() => {
    if (nodes.length === 0) {
      setVisible(false);
      return;
    }

    const anyNodeVisible = nodes.some(n => {
      if (n.x === undefined || n.y === undefined) return false;
      const sx = n.x * scale + offset.x;
      const sy = n.y * scale + offset.y;
      return sx >= 0 && sx <= width && sy >= 0 && sy <= height;
    });

    if (anyNodeVisible) {
      setVisible(false);
      return;
    }

    setVisible(true);
    const timeout = setTimeout(() => setVisible(false), 2000);
    return () => clearTimeout(timeout);
  }, [nodes, offset, scale, width, height]);

  return (
    <div
      className={`pointer-events-none absolute inset-0 flex items-center justify-center transition-opacity duration-700 ${
        visible ? 'opacity-100' : 'opacity-0'
      }`}
    >
      <div className="rounded-md bg-black/30 px-3 py-1.5 text-sm text-gray-100">
        Press <span className="font-semibold text-white">C</span> to recenter
      </div>
    </div>
  );
}
