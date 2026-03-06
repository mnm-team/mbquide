import { useState, useCallback } from 'react';
import { NodeType, Edge, OutputAdjustment, GraphState } from '../types';

export const useGraphState = () => {
  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);
  const [nodes, setNodes] = useState<NodeType[]>([]);
  const [edges, setEdges] = useState<Edge[]>([]);
  const [inputs, setInputs] = useState<number[]>([]);
  const [outputs, setOutputs] = useState<number[]>([]);
  const [adjustments, setAdjustments] = useState<Record<string, OutputAdjustment>>({});
  const [loading, setLoading] = useState(true);
  const [flowFocusable, setFlowFocusable] = useState(false);
  const [simulatable, setSimulatable] = useState(false);

  const getCurrentState = useCallback((): GraphState => ({
    nodes: [...nodes],
    edges: [...edges],
    inputs: [...inputs],
    outputs: [...outputs],
    adjustments: { ...adjustments },
  }), [nodes, edges, inputs, outputs, adjustments]);

  const updateState = useCallback((state: Partial<GraphState>) => {
    if (state.nodes !== undefined) setNodes(state.nodes);
    if (state.edges !== undefined) setEdges(state.edges);
    if (state.inputs !== undefined) setInputs(state.inputs);
    if (state.outputs !== undefined) setOutputs(state.outputs);
    if (state.adjustments !== undefined) setAdjustments(state.adjustments);
  }, []);

  return {
    // State
    selectedNodes,
    nodes,
    edges,
    inputs,
    outputs,
    adjustments,
    loading,
    flowFocusable,
    simulatable,
    
    // Setters
    setSelectedNodes,
    setNodes,
    setEdges,
    setInputs,
    setOutputs,
    setAdjustments,
    setLoading,
    setFlowFocusable,
    setSimulatable,
    
    // Helpers
    getCurrentState,
    updateState,
  };
};