import { useState, useEffect, useCallback, useRef } from 'react';
import { useNavigate } from 'react-router-dom';
import { MBQC_Graph } from '../../components/Graph';
import LoadingOverlay from '../../components/LoadingOverlay';
import { Edge, NodeType, OutputAdjustment, emptyOutputAdjustment, HistoryState } from './types';
import { useGraphState } from './hooks/useGraphState';
import { useGraphHistory } from './hooks/useGraphHistory';
import { useGraphApi } from './hooks/useGraphApi';
import { useGraphValidation } from './hooks/useGraphValidation';
import { ControlPanel } from '../../components/ControlPanel';
import { BuildingModeToggle } from './ui/buildingModeToggle';
import { getCenterOfNodes, hasNodeCrossedLayer } from './utils/positioning';
import {
  createLocalComplementationOperation,
  createPivotOperation,
  createZInsertionOperation,
  createZDeletionOperation,
  createRelabelingOperation,
  createRelabelingPlanarOperation,
  createGetFlowOperation,
  createFocusFlowOperation,
  createSimulateOperation,
  createSimplifyOperation,
} from './api/operations';

export default function MBQC_App() {
  const navigate = useNavigate();
  const [buildingMode, setBuildingMode] = useState(false);
  const [centerGraphTrigger, setCenterGraphTrigger] = useState(0);

  const {
    selectedNodes,
    nodes,
    edges,
    inputs,
    outputs,
    adjustments,
    loading,
    flowFocusable,
    simulatable,
    flowLayerLines,
    setSelectedNodes,
    setNodes,
    setEdges,
    setInputs,
    setOutputs,
    setAdjustments,
    setLoading,
    setFlowFocusable,
    setSimulatable,
    setFlowLayerLines,
    getCurrentState,
    updateState,
  } = useGraphState();

  const { saveToHistory, undo: undoHistory, redo: redoHistory, canUndo, canRedo } = useGraphHistory();

  const saveCurrentStateToHistory = useCallback(() => {
    saveToHistory(getCurrentState());
    setFlowLayerLines(null);
  }, [saveToHistory, getCurrentState, setFlowLayerLines]);

  const {
    fetchGraph,
    fetchGraphPreservePositions,
    writeGraph,
    runGraphOperation,
    orderNodesByFlow,
  } = useGraphApi({
    nodes,
    edges,
    inputs,
    outputs,
    adjustments,
    setNodes,
    setEdges,
    setInputs,
    setOutputs,
    setAdjustments,
    setLoading,
    setFlowFocusable,
    setSimulatable,
    setSelectedNodes,
    setFlowLayerLines,
    saveToHistory: saveCurrentStateToHistory,
  });

  const {
    isLCable,
    isPivotable,
    isZDeletable,
    canSimplify,
    fitForRelabeling,
    areNonPlanar,
  } = useGraphValidation(nodes, selectedNodes, edges, inputs, outputs);

  const handleUndo = useCallback(() => {
    const previousState = undoHistory(getCurrentState());
    if (previousState) {
      updateState(previousState);
      writeGraph(
        previousState.nodes,
        previousState.edges,
        previousState.inputs,
        previousState.outputs,
        previousState.adjustments
      );
    }
  }, [undoHistory, getCurrentState, updateState, writeGraph]);

  const handleRedo = useCallback(() => {
    const nextState = redoHistory(getCurrentState());
    if (nextState) {
      updateState(nextState);
      writeGraph(
        nextState.nodes,
        nextState.edges,
        nextState.inputs,
        nextState.outputs,
        nextState.adjustments
      );
    }
  }, [redoHistory, getCurrentState, updateState, writeGraph]);

  // Effect: Fetch graph
  useEffect(() => {
    fetchGraph();
  }, [fetchGraph]);

  // Effect: Loading Overlay
  useEffect(() => {
    if (!loading) return;
    const interval = setInterval(() => {
      fetchGraph();
    }, 3000);
    return () => clearInterval(interval);
  }, [loading, fetchGraph]);

  // Effect: Key handling
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      const isCtrl = e.ctrlKey || e.metaKey;

      // Ignore typing in inputs
      const tag = (e.target as HTMLElement).tagName;
      if (tag === 'INPUT' || tag === 'TEXTAREA') return;

      // Undo
      if (isCtrl && e.key === 'z' && !e.shiftKey) {
        e.preventDefault();
        handleUndo();
        return;
      }

      // Redo
      if (isCtrl && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
        e.preventDefault();
        handleRedo();
        return;
      }

      // Toggle building mode
      if (e.key === 'b' || e.key === 'B') {
        e.preventDefault();
        setBuildingMode(prev => !prev);
      }
    };

    window.addEventListener('keydown', handleKeyDown);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [handleUndo, handleRedo]);

  const handlePrintNodes = useCallback(() => {
    console.log('Selected Nodes:', selectedNodes);
    setSelectedNodes([]);
  }, [selectedNodes, setSelectedNodes]);

  const handleLocalComplementation = useCallback(() => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createLocalComplementationOperation(selectedNodes[0].id));
  }, [selectedNodes, runGraphOperation]);

  const handlePivot = useCallback(() => {
    if (selectedNodes.length !== 2) return;
    runGraphOperation(createPivotOperation(selectedNodes[0].id, selectedNodes[1].id));
  }, [selectedNodes, runGraphOperation]);

  const handleZInsertion = useCallback(() => {
    const nodeIDs = selectedNodes.map(n => n.id);
    let zPosition = getCenterOfNodes(selectedNodes);
    if (nodeIDs.length === 1) {
      zPosition = { x: zPosition.x + 50, y: zPosition.y - 50 };
    }
    runGraphOperation(
      createZInsertionOperation(nodeIDs),
      { pos: [zPosition.x, zPosition.y] }
    );
  }, [selectedNodes, runGraphOperation]);

  const handleZDeletion = useCallback(() => {
    const nodeIDs = selectedNodes.map(n => n.id);
    runGraphOperation(
      createZDeletionOperation(nodeIDs),
      { deleted_indices: nodeIDs }
    );
  }, [selectedNodes, runGraphOperation]);

  const handleRelabeling = useCallback(() => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createRelabelingOperation(selectedNodes[0].id));
  }, [selectedNodes, runGraphOperation]);

  const handleRelabelingPlanar = useCallback((basis: string = "") => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createRelabelingPlanarOperation(selectedNodes[0].id, basis));
  }, [selectedNodes, runGraphOperation]);

  const handleSimulate = useCallback(async () => {
    await runGraphOperation(createSimulateOperation());
    // Carry the editor's current layout over to the simulator's initial render.
    navigate('/SIM', { state: { editorNodes: nodes } });
  }, [runGraphOperation, navigate, nodes]);

  const handleGetFlow = useCallback(async () => {
    try {
      const operation = createGetFlowOperation();
      const response = await fetch('http://localhost:18080/api/graph', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(operation),
        credentials: 'include',
      });

      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);

      const data = await response.json();
      const flow = data.flow;

      if (flow?.ok) {
        console.log('The graph has pauli flow!');
        console.log(`\tCorrf: ${JSON.stringify(flow.corrf)}`);
        console.log(`\tOdd neigbors corrf: ${JSON.stringify(flow.oddNcorrf)}`);
        console.log(`\tDepths: ${JSON.stringify(flow.depths)}`);
        // Preserve the pre-flow layout so undo can remove the flow again.
        saveCurrentStateToHistory();
        orderNodesByFlow(flow.depths, flow.corrf, flow.oddNcorrf);
        setCenterGraphTrigger(prev => prev + 1);
        setFlowFocusable(true);
        setSimulatable(true);
      } else {
        alert('The graph has NO pauli flow!');
        setFlowFocusable(false);
      }
    } catch (error) {
      console.error('Error getting flow information:', error);
    }
  }, [orderNodesByFlow, setFlowFocusable, setSimulatable, saveCurrentStateToHistory]);

  const handleFocusFlow = useCallback(async () => {
    try {
      const operation = createFocusFlowOperation();
      const response = await fetch('http://localhost:18080/api/graph', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(operation),
        credentials: 'include',
      });

      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);

      const data = await response.json();
      const flow = data.flow;

      if (flow?.ok) {
        setFlowFocusable(false);
        // Preserve the pre-focus layout so undo can revert to it.
        saveCurrentStateToHistory();
        orderNodesByFlow(flow.depths, flow.corrf, flow.oddNcorrf);
      } else {
        console.log('The graph has no flow after focus operation!');
        setFlowFocusable(false);
      }
    } catch (error) {
      console.error('Error focusing flow:', error);
    }
  }, [orderNodesByFlow, setFlowFocusable, saveCurrentStateToHistory]);

  // Snapshot taken at drag-start, so that if the drag destroys the flow the correct state is pushed onto the undo stack (the graph state at drag-end already reflects the crossed-over position, since dragging mutates nodes in place).
  const dragStartSnapshotRef = useRef<HistoryState | null>(null);

  const handleNodeDragStart = useCallback(() => {
    dragStartSnapshotRef.current = getCurrentState();
  }, [getCurrentState]);

  // Dragging a node across a flow-layer boundary invalidates the flow.
  const handleNodeDragEnd = useCallback((draggedNodes: NodeType[]) => {
    if (!flowLayerLines) return;
    if (hasNodeCrossedLayer(draggedNodes, flowLayerLines)) {
      if (dragStartSnapshotRef.current) {
        saveToHistory(dragStartSnapshotRef.current);
        dragStartSnapshotRef.current = null;
      }
      setFlowLayerLines(null);
      setSimulatable(false);
    }
  }, [flowLayerLines, setFlowLayerLines, setSimulatable, saveToHistory]);

  const handleNodeDrop = useCallback(async (
    droppedNode?: NodeType,
    x?: number,
    y?: number,
    isInput: boolean = false
  ) => {
    if (!droppedNode) return;

    saveCurrentStateToHistory();

    const maxId = Math.max(...nodes.map(n => n.id), -1);
    const nextId = maxId + 1;

    const newNode: NodeType = {
      ...droppedNode,
      id: nextId,
      x,
      y,
      fx: null,
      fy: null,
    };

    const updatedNodes = [...nodes, newNode];
    setNodes(updatedNodes);

    const isOutput = newNode.basis === "OUTPUT";
    const updatedOutputs = isOutput ? [...outputs, newNode.id] : outputs;
    const updatedAdjustments = isOutput
      ? { ...adjustments, [newNode.id.toString()]: emptyOutputAdjustment }
      : adjustments;

    if (isOutput) {
      setOutputs(updatedOutputs);
      setAdjustments(updatedAdjustments);
    }

    const updatedInputs = isInput ? [...inputs, newNode.id] : inputs;
    setInputs(updatedInputs);

    await writeGraph(updatedNodes, edges, updatedInputs, updatedOutputs, updatedAdjustments);
    fetchGraphPreservePositions(updatedNodes);
  }, [
    nodes, edges, inputs, outputs, adjustments,
    setNodes, setOutputs, setAdjustments, setInputs,
    writeGraph, fetchGraphPreservePositions, saveCurrentStateToHistory,
  ]);

  const handleNodeDelete = useCallback(async (nodesToDelete?: NodeType[]) => {
    if (!nodesToDelete || nodesToDelete.length === 0) return;

    saveCurrentStateToHistory();

    const deleteIds = new Set(nodesToDelete.map(n => n.id));

    // Step 1: Filter remaining nodes
    const remainingNodes = nodes.filter(node => !deleteIds.has(node.id));

    // Step 2: Build ID remap
    const sortedRemaining = [...remainingNodes].sort((a, b) => a.id - b.id);

    const idMap = new Map<number, number>();
    sortedRemaining.forEach((node, newIndex) => {
      idMap.set(node.id, newIndex);
    });

    // Step 3: Apply remap to nodes
    const updatedNodes = sortedRemaining.map(node => ({
      ...node,
      id: idMap.get(node.id)!,
    }));
    setNodes(updatedNodes);

    // Step 4: Update edges
    const updatedEdges = edges
      .filter(edge => !deleteIds.has(edge.source) && !deleteIds.has(edge.target))
      .map(edge => ({
        source: idMap.get(edge.source)!,
        target: idMap.get(edge.target)!,
        colorCode: edge.colorCode,
      }));
    setEdges(updatedEdges);

    // Step 5: Inputs
    const updatedInputs = inputs
      .filter(id => !deleteIds.has(id))
      .map(id => idMap.get(id)!);
    setInputs(updatedInputs);

    // Step 6: Outputs
    const updatedOutputs = outputs
      .filter(id => !deleteIds.has(id))
      .map(id => idMap.get(id)!);
    setOutputs(updatedOutputs);

    // Step 7: Adjustments
    const updatedAdjustments: Record<string, OutputAdjustment> = {};
    Object.entries(adjustments || {}).forEach(([key, value]) => {
      const oldId = parseInt(key, 10);
      if (deleteIds.has(oldId)) return;

      const newId = idMap.get(oldId);
      if (newId !== undefined) {
        updatedAdjustments[newId.toString()] = value;
      }
    });
    setAdjustments(updatedAdjustments);

    await writeGraph(
      updatedNodes,
      updatedEdges,
      updatedInputs,
      updatedOutputs,
      updatedAdjustments
    );

    fetchGraphPreservePositions(updatedNodes);
  }, [
    nodes, edges, inputs, outputs, adjustments,
    setNodes, setEdges, setInputs, setOutputs, setAdjustments,
    writeGraph, fetchGraphPreservePositions, saveCurrentStateToHistory,
  ]);

  const handleEdgeCreation = useCallback(async (newEdge?: Edge) => {
    if (!newEdge) return;

    saveCurrentStateToHistory();

    const exists = edges.some(e =>
      (e.source === newEdge.source && e.target === newEdge.target) ||
      (e.source === newEdge.target && e.target === newEdge.source)
    );

    const updatedEdges = exists
      ? edges.filter(e => !(
          (e.source === newEdge.source && e.target === newEdge.target) ||
          (e.source === newEdge.target && e.target === newEdge.source)
        ))
      : [...edges, newEdge];

    setEdges(updatedEdges);

    await writeGraph(nodes, updatedEdges, inputs, outputs, adjustments);
    fetchGraphPreservePositions(nodes);
  }, [
    edges, nodes, inputs, outputs, adjustments,
    setEdges, writeGraph, fetchGraphPreservePositions, saveCurrentStateToHistory,
  ]);

  const handlePhaseSet = useCallback(async (nodeToSet?: NodeType, angleToSet?: number) => {
    if (!nodeToSet || angleToSet == null) return;

    saveCurrentStateToHistory();

    const updatedNodes = nodes.map(node =>
      node.id === nodeToSet.id ? { ...node, phase: angleToSet.toString() } : node
    );
    setNodes(updatedNodes);

    await writeGraph(updatedNodes, edges, inputs, outputs, adjustments);
    fetchGraphPreservePositions(nodes);
  }, [
    nodes, edges, inputs, outputs, adjustments,
    setNodes, writeGraph, fetchGraphPreservePositions, saveCurrentStateToHistory,
  ]);

  const handleResetGraph = useCallback(async () => {
    saveCurrentStateToHistory();
    setNodes([]);
    setEdges([]);
    setInputs([]);
    setOutputs([]);
    setAdjustments({});
    await writeGraph([], [], [], [], {});
    fetchGraph();
  }, [
    setNodes, setEdges, setInputs, setOutputs, setAdjustments,
    writeGraph, fetchGraph, saveCurrentStateToHistory,
  ]);

  const handleSimplifyGraph = useCallback(async () => {
    await runGraphOperation(createSimplifyOperation());
    fetchGraph();
  }, [runGraphOperation, fetchGraph]);

  return (
    <div className="relative h-screen w-screen overflow-hidden bg-[#111]">
      <LoadingOverlay isLoading={loading} />

      <BuildingModeToggle
        buildingMode={buildingMode}
        setBuildingMode={setBuildingMode}
      />

      {/* GRAPH LAYER */}
      <div className="relative h-full w-full">
        <MBQC_Graph
          nodes={nodes}
          edges={edges}
          inputs={inputs}
          outputs={outputs}
          outputAdjustments={adjustments}
          onSelectionChange={setSelectedNodes}
          runLocalComplementation={handleLocalComplementation}
          runRelabeling={handleRelabeling}
          runRelabelingPlanar={handleRelabelingPlanar}
          onNodeDrop={handleNodeDrop}
          onNodeDragStart={handleNodeDragStart}
          onNodeDragEnd={handleNodeDragEnd}
          onNodeDelete={handleNodeDelete}
          onCreateNewEdge={handleEdgeCreation}
          onPhaseSubmit={handlePhaseSet}
          buildingMode={buildingMode}
          centerGraphTrigger={centerGraphTrigger}
          flowLayerLines={flowLayerLines}
        />

        {/* OVERLAY CONTROL PANEL */}
        <div className="pointer-events-none absolute bottom-4 left-1/2 z-[1000] w-full -translate-x-1/2">
          <div className="pointer-events-auto flex justify-center px-2">
            <div className="w-fit max-w-[100vw] rounded-2xl border border-black/10 bg-white/10 p-2 backdrop-blur-xl">
              <ControlPanel
                selectedCount={selectedNodes.length}
                canUndo={canUndo}
                canRedo={canRedo}
                onUndo={handleUndo}
                onRedo={handleRedo}

                {...(!buildingMode && {
                  onSimplifyGraph: handleSimplifyGraph,
                  onLocalComplementation: handleLocalComplementation,
                  onPivot: handlePivot,
                  onZInsertion: handleZInsertion,
                  onZDeletion: handleZDeletion,
                  onRelabeling: handleRelabeling,
                  onRelabelingPlanar: handleRelabelingPlanar,
                  isLCable: isLCable(),
                  isPivotable: isPivotable(),
                  isZDeletable: isZDeletable(),
                  fitForRelabeling: fitForRelabeling(),
                  areNonPlanar: areNonPlanar(),
                  simplifyGraphDisabled: !canSimplify(),
                  onGetFlow: handleGetFlow,
                  onFocusFlow: handleFocusFlow,
                  onSimulate: handleSimulate,
                  flowFocusable,
                  simulatable,
                })}

                {...(buildingMode && {
                  onResetGraph: handleResetGraph,
                  resetGraphDisabled: nodes.length === 0,
                })}
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}