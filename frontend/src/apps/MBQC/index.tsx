import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { MBQC_Graph } from '../../components/Graph';
import LoadingOverlay from '../../components/LoadingOverlay';
import { Edge, NodeType, OutputAdjustment, emptyOutputAdjustment } from './types';
import { useGraphState } from './hooks/useGraphState';
import { useGraphHistory } from './hooks/useGraphHistory';
import { useGraphApi } from './hooks/useGraphApi';
import { useGraphValidation } from './hooks/useGraphValidation';
import { ControlPanel } from '../../components/ControlPanel';
import { BuildingModeToggle } from './ui/buildingModeToggle'
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
} from './api/operations';

export default function MBQC_App() {
  const navigate = useNavigate();
  const [buildingMode, setBuildingMode] = useState(false);
  
  // State management
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

  // History management
  const { saveToHistory, undo: undoHistory, redo: redoHistory, canUndo, canRedo } = useGraphHistory();

  const saveCurrentStateToHistory = () => {
    saveToHistory(getCurrentState());
  };

  // API operations
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

  // Validation
  const {
    isLCable,
    isPivotable,
    isZDeletable,
    fitForRelabeling,
    areNonPlanar,
  } = useGraphValidation(selectedNodes, edges, inputs, outputs);

  // Polling for initial load
  useEffect(() => {
    if (!loading) return;

    const interval = setInterval(() => {
      fetchGraph();
    }, 3000);

    return () => clearInterval(interval);
  }, [loading, fetchGraph]);

  // Initial fetch
  useEffect(() => {
    fetchGraph();
  }, [fetchGraph]);

  // Undo/Redo handlers
  const handleUndo = () => {
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
  };

  const handleRedo = () => {
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
  };

  // Operation handlers
  const handlePrintNodes = () => {
    console.log('Selected Nodes:', selectedNodes);
    setSelectedNodes([]);
  };

  const handleLocalComplementation = () => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createLocalComplementationOperation(selectedNodes[0].id));
  };

  const handlePivot = () => {
    if (selectedNodes.length !== 2) return;
    runGraphOperation(createPivotOperation(selectedNodes[0].id, selectedNodes[1].id));
  };

  const handleZInsertion = () => {
    const nodeIDs = selectedNodes.map(n => n.id);
    
    let zPosition = getCenterOfNodes(selectedNodes);
    if (nodeIDs.length === 1) {
      zPosition = { x: zPosition.x + 50, y: zPosition.y - 50 };
    }

    runGraphOperation(
      createZInsertionOperation(nodeIDs),
      { pos: [zPosition.x, zPosition.y] }
    );
  };

  const handleZDeletion = () => {
    if (selectedNodes.length !== 1) return;
    const nodeID = selectedNodes[0].id;
    runGraphOperation(
      createZDeletionOperation(nodeID),
      { deleted_index: nodeID }
    );
  };

  const handleRelabeling = () => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createRelabelingOperation(selectedNodes[0].id));
  };

  const handleRelabelingPlanar = () => {
    if (selectedNodes.length !== 1) return;
    runGraphOperation(createRelabelingPlanarOperation(selectedNodes[0].id));
  };

  const handleTransformToZX = async () => {
    await runGraphOperation(createTransformToZXOperation());
    navigate('/ZX');
  };

  const handleSimulate = async () => {
    await runGraphOperation(createSimulateOperation());
    navigate('/SIM');
  };

  const handleGetFlow = async () => {
    try {
      const operation = createGetFlowOperation();
      const response = await fetch('http://localhost:18080/api/graph', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(operation),
        credentials: 'include',
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

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
  };

  const handleFocusFlow = async () => {
    try {
      const operation = createFocusFlowOperation();
      const response = await fetch('http://localhost:18080/api/graph', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(operation),
        credentials: 'include',
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

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
  };

  const handleNodeDrop = async (droppedNode?: NodeType, x?: number, y?: number, isInput: boolean = false) => {

    if (!droppedNode) return;

    saveCurrentStateToHistory();
    
    const maxId = Math.max(...nodes.map(n => n.id), -1);
    const nextId = maxId + 1;
    
    const newNode: NodeType = {
      ...droppedNode,
      id: nextId,
      x: x,
      y: y,
      fx: null,
      fy: null,
    };
    
    const updatedNodes = [...nodes, newNode];
    setNodes(updatedNodes);

    const isOutput = newNode.basis === "OUTPUT"; 
    const updatedOutputs = isOutput ? [...outputs, newNode.id] : outputs;
    const updatedAdjustments = isOutput ? {...adjustments,  [newNode.id.toString()]: emptyOutputAdjustment} : adjustments;
    if (isOutput) {
      setOutputs(updatedOutputs);
      setAdjustments(updatedAdjustments);
    }

    const updatedInputs = isInput ? [...inputs, newNode.id] : inputs;
    setInputs(updatedInputs);
    
    await writeGraph(updatedNodes, edges, updatedInputs, updatedOutputs, updatedAdjustments);
    fetchGraphPreservePositions(updatedNodes);
  };


  const handleNodeDelete = async (nodeToDelete?: NodeType) => {

    console.log("handle delete");

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
  };


  const handleEdgeCreation = async (newEdge?: Edge) => {

    if (!newEdge) return;
    const exists = edges.some(e =>
      (e.source === newEdge.source && e.target === newEdge.target) ||
      (e.source === newEdge.target && e.target === newEdge.source)
    );

    saveCurrentStateToHistory();
          
    
    const updatedEdges = exists
      ? edges.filter(e =>
          !(
            (e.source === newEdge.source && e.target === newEdge.target) ||
            (e.source === newEdge.target && e.target === newEdge.source)
          )
        )
      : [...edges, newEdge];
      
    setEdges(updatedEdges);
    
    await writeGraph(nodes, updatedEdges, inputs, outputs, adjustments);
    fetchGraphPreservePositions(nodes);
  };

  const handlePhaseSet = async (nodeToSet?: NodeType, angleToSet?: number) => {
    if (!nodeToSet) return;
    if (angleToSet == null) return;

    const idToSet = nodeToSet.id;
  
    saveCurrentStateToHistory();

    const updatedNodes = nodes
      .map(node => (node.id == idToSet ? { ...node, phase: angleToSet.toString() } : node));
    setNodes(updatedNodes);

    await writeGraph(updatedNodes, edges, inputs, outputs, adjustments);
    fetchGraphPreservePositions(nodes);
  };

  const handleResetGraph = async () => {
    saveCurrentStateToHistory();
    setNodes([]);
    setEdges([]);
    setInputs([]);
    setOutputs([]);
    setAdjustments({});
    await writeGraph([], [], [], [], {});
    fetchGraph();
  }

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
        runRelabelingPlanar={(basis) => {
          if (selectedNodes.length !== 1) return;
          runGraphOperation(createRelabelingPlanarOperation(selectedNodes[0].id, basis));
        }}
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
          // onPrintNodes: handlePrintNodes,
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