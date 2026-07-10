import { useState, useCallback } from 'react';
import { NodeType, Edge, OutputAdjustment, GraphState, LayerLine } from '../types';

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
  const [flowLayerLines, setFlowLayerLines] = useState<LayerLine[] | null>(null);

  const getCurrentState = useCallback((): GraphState => ({
    nodes: nodes.map(node => ({ ...node })),
    edges: [...edges],
    inputs: [...inputs],
    outputs: [...outputs],
    adjustments: { ...adjustments },
    flowLayerLines: flowLayerLines ? flowLayerLines.map(line => ({ ...line })) : null,
    simulatable,
    flowFocusable,
  }), [nodes, edges, inputs, outputs, adjustments, flowLayerLines, simulatable, flowFocusable]);

  const updateState = useCallback((state: Partial<GraphState>) => {
    if (state.nodes !== undefined) setNodes(state.nodes);
    if (state.edges !== undefined) setEdges(state.edges);
    if (state.inputs !== undefined) setInputs(state.inputs);
    if (state.outputs !== undefined) setOutputs(state.outputs);
    if (state.adjustments !== undefined) setAdjustments(state.adjustments);
    if (state.flowLayerLines !== undefined) setFlowLayerLines(state.flowLayerLines);
    if (state.simulatable !== undefined) setSimulatable(state.simulatable);
    if (state.flowFocusable !== undefined) setFlowFocusable(state.flowFocusable);
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
    flowLayerLines,

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
    setFlowLayerLines,

    // Helpers
    getCurrentState,
    updateState,
  };
};