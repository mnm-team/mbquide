import { useState, useEffect, useCallback } from 'react';
import { useNavigate } from "react-router-dom";
import Statevector from '../components/Simulator/Statevector';
import StateInput from '../components/Simulator/StateInput';
import LoadingOverlay from '../components/LoadingOverlay';
import MBQC_Simulator from '../components/Simulator/index';
import { ControlPanel } from '../components/ControlPanel';
import { getDepthOrderedNodes } from './MBQC/utils/positioning'

import { NodeType, Edge, GraphApiResponse, FlowResult } from './MBQC/types';
import { Checkbox } from '../components/Buttons';


interface SimData {
  statevector: number[][];
  graph: GraphApiResponse;
  flow: FlowResult;
  readyToMeasure: number[];
  measured: number[];
  outcomes: [number,number][];  // unordered map<int,int> nodeID->0/1
  activeNodes: number[];
  activeEdges: [number, number][];
}


const SimulatorApp: React.FC = () => {
  const navigate = useNavigate();

  const [selectedNodes, setSelectedNodes] = useState<NodeType[]>([]);
  const [data, setData] = useState<SimData | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [nodes, setNodes] = useState<NodeType[]>([]);
  const [edges, setEdges] = useState<Edge[]>([]);
  const [inputs, setInputs] = useState<number[]>([]);
  const [outputs, setOutputs] = useState<number[]>([]);
  const [adjustments, setAdjustments] = useState<Record<string, { X: [string, number]; Z: [string, number] }>>({});
  const [readyToMeasure, setReadyToMeasure] = useState<number[]>([]);
  const [measured, setMeasured] = useState<number[]>([]);
  const [outcomes, setOutcomes] = useState<[number,number][]>([]);

  const [inputState, setInputState] = useState<string>("");
  const [random, setRandom] = useState<boolean>(true);

  const [activeNodes, setActiveNodes] = useState<number[]>([]);

  // HISTORY STACKS
  const [initData, setInitData] = useState<SimData | null>(null);
  const [undoStack, setUndoStack] = useState<SimData[]>([]);
  const [redoStack, setRedoStack] = useState<SimData[]>([]);
  const canUndo = undoStack.length > 0;
  const canRedo = redoStack.length > 0;

  const saveHistory = (currentData: SimData) => {
    setUndoStack(prev => [...prev, currentData]);
    setRedoStack([]);
  };

  const handleUndo = () => {
    if (!canUndo) return;
    const lastState = undoStack[undoStack.length - 1];
    setUndoStack(prev => prev.slice(0, prev.length - 1));
    if (data) setRedoStack(prev => [...prev, data]);
    updateSimulator(lastState);
  };

  const handleRedo = () => {
    if (!canRedo) return;
    const nextState = redoStack[redoStack.length - 1];
    setRedoStack(prev => prev.slice(0, prev.length - 1));
    if (data) setUndoStack(prev => [...prev, data]);
    updateSimulator(nextState);
  };

  const handleRandomCheckBoxClicked = (checked: boolean) => {
    setRandom(checked);
    resetGraph(undefined, checked);
  }


  const resetGraph = async (input?: string, checked?: boolean) => {
    setLoading(true);
    const body = {
      "init": true,
      "random": (checked === undefined || checked === null) ? random : checked,
      "input": (input === undefined || input === null) ? inputState : input,
    };
    
    const json: SimData = await postSim(body);
    updateSimulator(json);
    setLoading(false);
      
    setSelectedNodes([]);
  }

  const handleResetGraph = async () => {
    resetGraph();
  }
    
  const updateSimulator = (newData: SimData) => {
    
    setData(newData);
    if (!initData) setInitData(newData);

    setMeasured(newData.measured);
    setReadyToMeasure(newData.readyToMeasure);
    setOutcomes(newData.outcomes);
    setActiveNodes(newData.activeNodes);
    
    const graphData = newData.graph

    const filteredNodes: NodeType[] = (graphData.meas as [string, string][])
      .map(([basis, phase], index) => {
        return {
          id: index,
          basis,
          phase: phase || undefined,
        };
      });

    const filteredEdges: Edge[] = (graphData.edges as [number, number][])
      .map(([source, target]) => ({
        source,
        target,
        colorCode: newData.activeEdges.some(edge => 
          (edge[0] === source && edge[1] === target) || 
          (edge[0] === target && edge[1] === source)
        ) ? 1 : 3,
      }));


    setAdjustments(graphData.outAdj);


    setEdges(filteredEdges);
    setInputs(graphData.inputs || []);
    setOutputs(graphData.outputs || []);
    
    const flow = newData.flow;
    setNodes(getDepthOrderedNodes(filteredNodes, filteredEdges, flow.depths, flow.corrf, flow.oddNcorrf));

  }

  const handleInput = (state: string) => {
    setInputState(state);
    resetGraph(state);
  };

  const toMBQC = useCallback(async () => {
    setLoading(true)
    navigate("/MBQC")
    }, [navigate])


  useEffect(() => {
    const fetchData = async (): Promise<void> => {
      setLoading(true);
      const res = await fetch('http://localhost:18080/api/sim', {
        method: 'GET',
        headers: { 'Content-Type': 'application/json' },
        credentials: "include",
      })
      
      if (!res.ok) {
        console.error('Server returned', res.status, res.statusText)
        setLoading(false)
        return
      }

      const json: SimData = await res.json();
      console.log(json);
      updateSimulator(json);      
      setLoading(false);
    };

    fetchData();
  }, []);

  const postSim = async (body: unknown) => {
    const res = await fetch("http://localhost:18080/api/sim", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(body),
      credentials: "include",
    });

    if (!res.ok) {
      throw new Error(`HTTP error! status: ${res.status}`);
    }

    return res.json();
  };

  const measure = async (id: number) => {
    if (!selectedNodes) {
      console.error("No node selected");
      return;
    }
    
    setLoading(true);
    const body = {
      "measureNode": id,
    };
    
    const json: SimData = await postSim(body);
    updateSimulator(json);
    setLoading(false);
    setSelectedNodes([]);
  };


  const handleRunAll = async () => { 
    setLoading(true);
    const body = {
      "measureAll": true,
    };
    
    const json: SimData = await postSim(body);
    updateSimulator(json);
    setLoading(false);
    setSelectedNodes([]);
  };

  return (
    <div className="h-screen w-screen p-0 flex">
      <LoadingOverlay isLoading={loading} />

      {/* Random checkbox — top-left absolute */}
      <div className='absolute top-20 left-3 z-10'>
        <Checkbox
          label="Random"
          checked={random}
          onChange={handleRandomCheckBoxClicked}
          disabled={measured.length != 0}
        />
      </div>

      {/* Left/center column: StateInput on top, simulator below */}
      <div className="flex-1 flex flex-col h-screen" style={{ width: 'calc(100vw - 400px)' }}>
        <StateInput
          numQubits={inputs.length}
          onSubmit={handleInput}
          disableSubmit={measured.length != 0}
        />

        {data && data.graph && (
          <div className="flex-1 relative">
            <MBQC_Simulator
              nodes={nodes}
              edges={edges}
              inputs={inputs}
              outputs={outputs}
              outputAdjustments={adjustments}
              measured={measured}
              active={activeNodes}
              outcomes={outcomes}
              readyToMeasure={readyToMeasure}
              onSelectionChange={setSelectedNodes}
              measureOperation={measure}
              width={window.innerWidth - 400}
              height={window.innerHeight - 65}
            />
            <div className="absolute bottom-6 left-1/2 -translate-x-1/2 z-10 w-full">
              <ControlPanel
                selectedCount={0}
                canUndo={canUndo}
                canRedo={canRedo}
                onTransformToMBQC={toMBQC}
                // onUndo={handleUndo}
                // onRedo={handleRedo}
                onResetSim={handleResetGraph}
                canResetSim={measured.length != 0  }
                onRunAll={handleRunAll}
                canRunAll={readyToMeasure.length != 0}
              />
            </div>
          </div>
        )}
      </div>

      {/* Right column: Statevector */}
      {data && data.statevector && (
        <div className="w-100 h-screen overflow-auto z-100">
          <Statevector statevector={data.statevector} ids={activeNodes} />
        </div>
      )}
    </div>
  );

};

export default SimulatorApp;