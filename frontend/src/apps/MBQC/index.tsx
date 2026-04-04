import { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import { MBQC_Graph } from '../../components/Graph';
import LoadingOverlay from '../../components/LoadingOverlay';
import { Edge, NodeType, OutputAdjustment, emptyOutputAdjustment } from './types';
import { useGraphState } from './hooks/useGraphState';
import { useGraphHistory } from './hooks/useGraphHistory';
import { useGraphApi } from './hooks/useGraphApi';
import { useGraphValidation } from './hooks/useGraphValidation';
import { ControlPanel } from '../../components/ControlPanel';
import { BuildingModeToggle } from './ui/buildingModeToggle';
import { getCenterOfNodes } from './utils/positioning';
import {
  createLocalComplementationOperation,
  createPivotOperation,
  createZInsertionOperation,
  createZDeletionOperation,
  createRelabelingOperation,
  createRelabelingPlanarOperation,
  createGetFlowOperation,
  createFocusFlowOperation,
  createTransformToZXOperation,
  createSimulateOperation,
  createSimplifyOperation,
} from './api/operations';

export default function MBQC_App() {
  const navigate = useNavigate();
  const [buildingMode, setBuildingMode] = useState(false);

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
    setSelectedNodes,
    setNodes,
    setEdges,
    setInputs,
    setOutputs,
    setAdjustments,
    setLoading,
    setFlowFocusable,
    setSimulatable,
    getCurrentState,
    updateState,
  } = useGraphState();

  const { saveToHistory, undo: undoHistory, redo: redoHistory, canUndo, canRedo } = useGraphHistory();

  const saveCurrentStateToHistory = useCallback(() => {
    saveToHistory(getCurrentState());
  }, [saveToHistory, getCurrentState]);

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

  useEffect(() => {
    if (!loading) return;
    const interval = setInterval(() => {
      fetchGraph();
    }, 3000);
    return () => clearInterval(interval);
  }, [loading, fetchGraph]);

  useEffect(() => {
    fetchGraph();
  }, [fetchGraph]);

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

  const handleTransformToZX = useCallback(async () => {
    await runGraphOperation(createTransformToZXOperation());
    navigate('/ZX');
  }, [runGraphOperation, navigate]);

  const handleSimulate = useCallback(async () => {
    await runGraphOperation(createSimulateOperation());
    navigate('/SIM');
  }, [runGraphOperation, navigate]);

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
        orderNodesByFlow(flow.depths, flow.corrf, flow.oddNcorrf);
        setFlowFocusable(true);
        setSimulatable(true);
      } else {
        alert('The graph has NO pauli flow!');
        setFlowFocusable(false);
      }
    } catch (error) {
      console.error('Error getting flow information:', error);
    }
  }, [orderNodesByFlow, setFlowFocusable, setSimulatable]);

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
        orderNodesByFlow(flow.depths, flow.corrf, flow.oddNcorrf);
      } else {
        console.log('The graph has no flow after focus operation!');
        setFlowFocusable(false);
      }
    } catch (error) {
      console.error('Error focusing flow:', error);
    }
  }, [orderNodesByFlow, setFlowFocusable]);

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

  const handleNodeDelete = useCallback(async (nodeToDelete?: NodeType) => {
    if (!nodeToDelete) return;

    saveCurrentStateToHistory();

    const deletedId = nodeToDelete.id;

    const updatedNodes = nodes
      .filter(node => node.id !== deletedId)
      .map(node => (node.id > deletedId ? { ...node, id: node.id - 1 } : node));
    setNodes(updatedNodes);

    const updatedEdges = edges
      .filter(edge => edge.source !== deletedId && edge.target !== deletedId)
      .map(edge => ({
        source: edge.source > deletedId ? edge.source - 1 : edge.source,
        target: edge.target > deletedId ? edge.target - 1 : edge.target,
        colorCode: edge.colorCode,
      }));
    setEdges(updatedEdges);

    const updatedInputs = inputs
      .filter(id => id !== deletedId)
      .map(id => (id > deletedId ? id - 1 : id));
    setInputs(updatedInputs);

    const updatedOutputs = outputs
      .filter(id => id !== deletedId)
      .map(id => (id > deletedId ? id - 1 : id));
    setOutputs(updatedOutputs);

    const updatedAdjustments: Record<string, OutputAdjustment> = {};
    Object.entries(adjustments || {}).forEach(([key, value]) => {
      const numericKey = parseInt(key, 10);
      if (numericKey === deletedId) return;
      const newKey = numericKey > deletedId ? (numericKey - 1).toString() : key;
      updatedAdjustments[newKey] = value;
    });
    setAdjustments(updatedAdjustments);

    await writeGraph(updatedNodes, updatedEdges, updatedInputs, updatedOutputs, updatedAdjustments);
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
    <div style={{ textAlign: 'center', padding: '20px' }}>
      <LoadingOverlay isLoading={loading} />

      <BuildingModeToggle
        buildingMode={buildingMode}
        setBuildingMode={setBuildingMode}
      />

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
        onNodeDelete={handleNodeDelete}
        onCreateNewEdge={handleEdgeCreation}
        onPhaseSubmit={handlePhaseSet}
        buildingMode={buildingMode}
      />

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
          onTransformToZX: handleTransformToZX,
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
  );
}