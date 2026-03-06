import React from 'react';
import { NodeType } from '../types';
import { ActionButton } from '../../Buttons';
import { actionIcon } from '../../Icons';

type ContextMenuProps = {
  visible: boolean;
  x: number;
  y: number;
  node: NodeType | null;
  selectedNodes: NodeType[];
  onRelabeling: () => void;
  onRelabelingPlanar: (basis: string) => void;
};

const fitForRelabeling = (nodes: NodeType[]) => {
  return nodes.every(n => 
    (n.basis === "XY" || n.basis === "XZ" || n.basis === "YZ") &&
    (!n.phase || ["", "\u03c0", "2\u03c0", "\u03c0/2", "3\u03c0/2"].includes(n.phase))
  );
};

export const ContextMenu: React.FC<ContextMenuProps> = ({
  visible,
  x,
  y,
  node,
  selectedNodes,
  onRelabeling,
  onRelabelingPlanar,
}) => {
  if (!visible || !node) return null;

  return (
    <div
      style={{
        position: "absolute",
        top: y,
        left: x,
        backgroundColor: "#fff",
        border: "1px solid #ccc",
        borderRadius: "4px",
        boxShadow: "0 2px 6px rgba(0,0,0,0.2)",
        padding: "8px",
        zIndex: 1000,
      }}
      onClick={(e) => e.stopPropagation()}
    >
      <div><strong>Node ID:</strong> {node.id}</div>
      <div><strong>Basis:</strong> {node.basis}</div>
      <div><strong>Phase:</strong> {node.phase ?? "–"}</div>
      <div><strong>Correction Set:</strong> {node.correctionSet?.join(", ") ?? "–"}</div>
        
      <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
        {node.basis === "X" && (
          <>
            <ActionButton
              onClick={() => onRelabelingPlanar("XZ")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to XZ'}
            />
            <ActionButton
              onClick={() => onRelabelingPlanar("XY")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to XY'}
            />
          </>
        )}

        {node.basis === "Y" && (
          <>
            <ActionButton
              onClick={() => onRelabelingPlanar("YZ")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to YZ'}
            />
            <ActionButton
              onClick={() => onRelabelingPlanar("XY")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to XY'}
            />
          </>
        )}

        {node.basis === "Z" && (
          <>
            <ActionButton
              onClick={() => onRelabelingPlanar("XZ")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to XZ'}
            />
            <ActionButton
              onClick={() => onRelabelingPlanar("YZ")}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel to YZ'}
            />
          </>
        )}

        {fitForRelabeling(selectedNodes) && (
            <ActionButton
              onClick={onRelabeling}
              disabled={selectedNodes.length !== 1}
              label={actionIcon + ' Relabel'}
            />
        )}
      </div>
    </div>
  );
};