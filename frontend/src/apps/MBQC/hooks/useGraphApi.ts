import { useCallback } from 'react';
import { NodeType, Edge, OutputAdjustment, GraphApiResponse, UpdateGraphOptions } from '../types';
import { fetchGraphFromBackend, writeGraphToBackend, executeGraphOperation } from '../api/graphApi';
import { 
  positionNodes,
  getDepthOrderedNodes,
  getNodePosition
} from '../utils/positioning';
import { 
  transformApiResponseToNodes, 
  transformApiResponseToEdges, 
  transformNodesForUpdate,
  preserveNodePositions 
} from '../utils/transforms';

type UseGraphApiProps = {
  nodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  adjustments: Record<string, OutputAdjustment>;
  setNodes: (nodes: NodeType[]) => void;
  setEdges: (edges: Edge[]) => void;
  setInputs: (inputs: number[]) => void;
  setOutputs: (outputs: number[]) => void;
  setAdjustments: (adjustments: Record<string, OutputAdjustment>) => void;
  setLoading: (loading: boolean) => void;
  setFlowFocusable: (focusable: boolean) => void;
  setSimulatable: (simulatable: boolean) => void;
  setSelectedNodes: (nodes: NodeType[]) => void;
  saveToHistory: () => void;
};

export const useGraphApi = ({
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
  saveToHistory,
}: UseGraphApiProps) => {

  const fetchGraph = useCallback(async () => {
    setLoading(true);

    try {
      const data = await fetchGraphFromBackend();
      
      
      const filteredNodes = transformApiResponseToNodes(data);
      const filteredEdges = transformApiResponseToEdges(data);

      const positionedNodes = positionNodes(filteredNodes, filteredEdges, data.inputs);

      setNodes(positionedNodes);
      setEdges(filteredEdges);
      setInputs(data.inputs || []);
      setOutputs(data.outputs || []);
      setAdjustments(data.outAdj);
    } catch (error) {
      console.error('Error fetching graph:', error);
    } finally {
      setLoading(false);
    }
  }, [setNodes, setEdges, setInputs, setOutputs, setAdjustments, setLoading]);

  const fetchGraphPreservePositions = useCallback(
    async (nodesSnapshot: NodeType[]) => {
      try {
        const data = await fetchGraphFromBackend();

        const filteredEdges = transformApiResponseToEdges(data);
        const filteredNodesUnpositioned = transformApiResponseToNodes(data);

        const defaultPositions = getNodePosition(filteredNodesUnpositioned, filteredEdges, data.inputs);

        const filteredNodes = preserveNodePositions(
          data,
          nodesSnapshot,
          defaultPositions
        );


        setNodes(filteredNodes);
        setEdges(filteredEdges);
        setInputs(data.inputs || []);
        setOutputs(data.outputs || []);
        setAdjustments(data.outAdj);
      } catch (error) {
        console.error(error);
      }
    },
    [setNodes, setEdges, setInputs, setOutputs, setAdjustments]
  );

  const writeGraph = useCallback(async (
    customNodes = nodes,
    customEdges = edges,
    customInputs = inputs,
    customOutputs = outputs,
    customAdjustments = adjustments,
  ) => {
    try {
      await writeGraphToBackend(
        customNodes,
        customEdges,
        customInputs,
        customOutputs,
        customAdjustments
      );
    } catch (error) {
      console.error('Error writing graph to backend:', error);
    }
  }, [nodes, edges, inputs, outputs, adjustments]);

  const updateGraph = useCallback((
    data: GraphApiResponse,
    options: UpdateGraphOptions = {}
  ) => {
    const filteredNodes = transformNodesForUpdate(data, nodes, options);
    const filteredEdges = transformApiResponseToEdges(data);

    setNodes(filteredNodes);
    setEdges(filteredEdges);
    setInputs(data.inputs || []);
    setOutputs(data.outputs || []);
    setAdjustments(data.outAdj);
  }, [nodes, setNodes, setEdges, setInputs, setOutputs, setAdjustments]);

  const runGraphOperation = useCallback(async (
    operationParams: Record<string, any>,
    graphOptions: UpdateGraphOptions = {},
  ) => {
    saveToHistory();

    try {
      const updatedGraphData = await executeGraphOperation(operationParams);
      updateGraph(updatedGraphData, graphOptions);
      setFlowFocusable(false);
      setSelectedNodes([]);
    } catch (error) {
      console.error(`Error running graph operation:`, error);
    }
  }, [saveToHistory, updateGraph, setFlowFocusable, setSelectedNodes]);

  const orderNodesByFlow = useCallback((depths: number[], corrf: Record<number, number[]>, oddNcorrf: Record<number, number[]>) => {
    const orderedNodes = getDepthOrderedNodes(nodes, edges, depths, corrf, oddNcorrf);
    setNodes(orderedNodes);
  }, [nodes, setNodes]);

  return {
    fetchGraph,
    fetchGraphPreservePositions,
    writeGraph,
    updateGraph,
    runGraphOperation,
    orderNodesByFlow,
  };
};