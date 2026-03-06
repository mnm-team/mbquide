import { useEffect, useRef, useState } from 'react';
import { GraphProps, NodeType } from './types';
import { useGraphSimulation } from './hooks/useGraphSimulation';
import { useContextMenu } from './hooks/useContextMenu';
import { useOutputTables } from './hooks/useOutputTables';
import { ContextMenu } from './ui/ContextMenu';
import { OutputTable } from './ui/OutputTable';
import { PhaseInputModal } from './ui/phaseInputMode';
import { BACKGROUND_COLOR } from './utils/colors';

export function MBQC_Graph({
  nodes: mainNodes,
  edges,
  onSelectionChange,
  inputs,
  outputs,
  outputAdjustments = {},
  runLocalComplementation,
  runRelabeling,
  runRelabelingPlanar,
  onNodeDrop,
  onNodeDelete,
  onCreateNewEdge,
  onPhaseSubmit,
  buildingMode = false,
}: GraphProps) {
  const selectedNodesRef = useRef<NodeType[]>([]);
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);
  const [phaseModalNode, setPhaseModalNode] = useState<NodeType | null>(null);
  
  const { contextMenu, setContextMenu } = useContextMenu();
  const { outputTables, updateOutputTables } = useOutputTables();

  const handleNodeDoubleClick = (node: NodeType) => {
    setPhaseModalNode(node);
  };

  const handlePhaseModalClose = () => {
    setPhaseModalNode(null);
  };

  const handlePhaseSubmit = (angle: number) => {
    if (!phaseModalNode) return;
    if (onPhaseSubmit) {
      onPhaseSubmit(phaseModalNode, angle);
    }
    setPhaseModalNode(null);
  };

  const { svgRef } = useGraphSimulation({
    mainNodes,
    edges,
    inputs,
    outputs,
    selectedNodes,
    selectedNodesRef,
    setSelectedNodes,
    setContextMenu,
    contextMenu,
    onSelectionChange,
    onNodeDrop,
    onNodeDelete,
    onCreateNewEdge,
    updateOutputTables,
    buildingMode,
    onNodeDoubleClick: handleNodeDoubleClick,
  });

  // Keep ref in sync with state
  useEffect(() => {
    selectedNodesRef.current = selectedNodes;
  }, [selectedNodes]);

  // Context menu handlers
  const handleLocalComplementation = () => {
    if (!runLocalComplementation) return;
    runLocalComplementation();
    setContextMenu({ ...contextMenu, visible: false });
  };

  const handleRelabeling = () => {
    if (!runRelabeling) return;
    runRelabeling();
    setContextMenu({ ...contextMenu, visible: false });
  };

  const handleRelabelingPlanar = (basis: string = "") => {
    if (!runRelabelingPlanar) return;
    runRelabelingPlanar(basis);
    setContextMenu({ ...contextMenu, visible: false });
  };

  return (
    <div style={{ position: 'relative' }}>
      <svg
        ref={svgRef}
        viewBox="0 0 1920 900"
        width="100%"
        height="auto"
        preserveAspectRatio="xMidYMid meet"
        style={{ backgroundColor: BACKGROUND_COLOR }}
      />
     
      {/* Output Tables */}
      {Object.entries(outputTables).map(([nodeId, position]) => {
        const node = mainNodes.find(n => n.id === parseInt(nodeId));
        const adjustment = outputAdjustments[parseInt(nodeId)];
        if (!node || !adjustment) return null;
        
        return (
          <OutputTable
            key={nodeId}
            nodeId={parseInt(nodeId)}
            position={position}
            adjustment={adjustment}
            svgRef={svgRef}
          />
        );
      })}

      {/* Context Menu */}
      <ContextMenu
        visible={contextMenu.visible}
        x={contextMenu.x}
        y={contextMenu.y}
        node={contextMenu.node}
        selectedNodes={selectedNodes}
        onRelabeling={handleRelabeling}
        onRelabelingPlanar={handleRelabelingPlanar}
      />

      <PhaseInputModal
        node={phaseModalNode}
        isOpen={phaseModalNode !== null}
        onClose={handlePhaseModalClose}
        onSubmit={handlePhaseSubmit}
      />
    </div>
  );
}


export function ZX_Graph({
  nodes: mainNodes,
  edges,
  inputs,
  outputs,
}: GraphProps) {
  const selectedNodesRef = useRef<NodeType[]>([]);
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);
  
  const { svgRef } = useGraphSimulation({
    mainNodes,
    edges,
    inputs,
    outputs,
    selectedNodes,
    selectedNodesRef,
    setSelectedNodes,
    ignoreExamples: true,
    classicZXcolors: true,
  });

  return (
    <div style={{ position: 'relative' }}>
      <svg
        ref={svgRef}
        viewBox="0 0 1920 900"
        width="100%"
        height="auto"
        preserveAspectRatio="xMidYMid meet"
        style={{ backgroundColor: BACKGROUND_COLOR }}
      />
    </div>
  );
}