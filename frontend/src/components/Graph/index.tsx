import { useEffect, useRef, useState } from 'react';
import { GraphProps, NodeType } from './types';
import { useGraphSimulation } from './hooks/useGraphSimulation';
import { useContextMenu } from './hooks/useContextMenu';
import { ContextMenu } from './ui/ContextMenu';
import { PhaseInputModal } from './ui/phaseInputMode';
import { BACKGROUND_COLOR } from './utils/colors';
import { useSvgPan } from './hooks/useSvgPan';

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

  const { svgRef, panGroupRef, setPanOffset } = useGraphSimulation({
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
    buildingMode,
    onNodeDoubleClick: handleNodeDoubleClick,
    outputAdjustments,
  });

  const { translate, onPointerDown, onPointerMove, onPointerUp } = useSvgPan(svgRef);


  // Sync translate
  useEffect(() => {
    if (!panGroupRef.current) return;
    panGroupRef.current.setAttribute(
      'transform',
      `translate(${translate.x}, ${translate.y})`
    );
    setPanOffset(translate);
  }, [translate]);

  // Keep ref in sync with state
  useEffect(() => {
    selectedNodesRef.current = selectedNodes;
  }, [selectedNodes]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      const tag = (e.target as HTMLElement).tagName;
      if (tag === 'INPUT' || tag === 'TEXTAREA') return;

      if (!buildingMode) return;

      if (e.key === 'Enter' && selectedNodes.length === 1) {
        e.preventDefault();
        setPhaseModalNode(selectedNodes[0]);
      }
    };

    window.addEventListener('keydown', handleKeyDown);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [buildingMode, selectedNodes]);

  
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
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerLeave={onPointerUp}
      />

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