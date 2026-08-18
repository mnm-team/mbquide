import { useEffect, useRef, useState } from 'react';
import { GraphProps, NodeType, UnfusionTarget } from './types';
import { useGraphSimulation } from './hooks/useGraphSimulation';
import { useContextMenu } from './hooks/useContextMenu';
import { ContextMenu } from './ui/ContextMenu';
import { PhaseInputModal } from './ui/phaseInputMode';
import { RecenterHint } from './ui/RecenterHint';
import { BACKGROUND_COLOR } from './utils/colors';
import { getBoundingCenter } from './utils/functions';
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
  onNodeDragStart,
  onNodeDragEnd,
  onNodeDelete,
  onCreateNewEdge,
  onPhaseSubmit,
  runYZUnfusion,
  onYZDragStart,
  onYZDragEnd,
  onYZAngleChange,
  buildingMode = false,
  centerGraphTrigger,
  flowLayerLines,
}: GraphProps) {
  const selectedNodesRef = useRef<NodeType[]>([]);
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);
  const [phaseModalNode, setPhaseModalNode] = useState<NodeType | null>(null);
  const [angleEditPair, setAngleEditPair] = useState<{ xy: NodeType; yz: NodeType } | null>(null);
  const [angleEditTarget, setAngleEditTarget] = useState<UnfusionTarget>('xy');

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

  const handleUnfusionHandleDoubleClick = (xyNode: NodeType, yzNode: NodeType) => {
    setAngleEditPair({ xy: xyNode, yz: yzNode });
    setAngleEditTarget('xy');
  };

  const handleAngleEditClose = () => {
    setAngleEditPair(null);
  };

  const handleAngleEditSubmit = (angle: number) => {
    if (!angleEditPair) return;
    if (onYZAngleChange) {
      onYZAngleChange(angleEditPair.xy, angleEditPair.yz, angle, angleEditTarget);
    }
    setAngleEditPair(null);
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
    flowLayerLines,
    onNodeDragStart,
    onNodeDragEnd,
    onYZDragStart,
    onYZDragEnd,
    onYZAngleChange,
    onUnfusionHandleDoubleClick: handleUnfusionHandleDoubleClick,
  });

  const { translate, setTranslate, onPointerDown, onPointerMove, onPointerUp } = useSvgPan(svgRef);

  // Sync translate
  useEffect(() => {
    if (!panGroupRef.current) return;
    panGroupRef.current.setAttribute(
      'transform',
      `translate(${translate.x}, ${translate.y})`
    );
    setPanOffset(translate);
  }, [translate]);

  // Reset Pan to Center
  useEffect(() => {
    if (mainNodes.length === 0) return;

    const center = getBoundingCenter(mainNodes);

    setTranslate({
      x: 960 - center.x,
      y: 540 - center.y,
    });
  }, [centerGraphTrigger, setTranslate]);

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

  const handleYZUnfusion = () => {
    if (!runYZUnfusion || !contextMenu.node) return;
    runYZUnfusion(contextMenu.node);
    setContextMenu({ ...contextMenu, visible: false });
  };

  return (
    <div style={{ position: 'relative' }}>
      <svg
        ref={svgRef}
        viewBox="0 0 1920 1080"
        width="100%"
        height="100%"
        preserveAspectRatio="xMidYMid meet"
        style={{ backgroundColor: BACKGROUND_COLOR }}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerLeave={onPointerUp}
      />

      <RecenterHint nodes={mainNodes} offset={translate} width={1920} height={1080} />

      {/* Context Menu */}
      <ContextMenu
        visible={contextMenu.visible}
        x={contextMenu.x}
        y={contextMenu.y}
        node={contextMenu.node}
        selectedNodes={selectedNodes}
        onRelabeling={handleRelabeling}
        onRelabelingPlanar={handleRelabelingPlanar}
        onYZUnfusion={runYZUnfusion ? handleYZUnfusion : undefined}
      />

      <PhaseInputModal
        node={phaseModalNode}
        isOpen={phaseModalNode !== null}
        onClose={handlePhaseModalClose}
        onSubmit={handlePhaseSubmit}
      />

      {angleEditPair && (
        <PhaseInputModal
          key={angleEditTarget}
          node={{ ...angleEditPair.xy, basis: 'XY' }}
          isOpen={angleEditPair !== null}
          onClose={handleAngleEditClose}
          onSubmit={handleAngleEditSubmit}
          title={
            angleEditTarget === 'xy'
              ? `Set the XY angle (Node ${angleEditPair.xy.id})`
              : `Set the YZ angle (Node ${angleEditPair.yz.id})`
          }
          targetOptions={[
            { value: 'xy', label: `Node ${angleEditPair.xy.id} (XY)` },
            { value: 'yz', label: `Node ${angleEditPair.yz.id} (YZ)` },
          ]}
          selectedTargetValue={angleEditTarget}
          onTargetChange={(value) => setAngleEditTarget(value as UnfusionTarget)}
        />
      )}
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
        viewBox="0 0 1920 1080"
        width="100%"
        height="100%"
        preserveAspectRatio="xMidYMid meet"
        style={{ backgroundColor: BACKGROUND_COLOR }}
      />
    </div>
  );
}