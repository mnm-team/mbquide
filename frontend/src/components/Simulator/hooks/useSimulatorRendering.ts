import { useEffect, useRef, useState } from 'react';
import * as d3 from 'd3';
import { NodeType, Edge, SimEdge } from '../types';

// Reuse rendering from MBQC_Graph
import { setupAllFilters } from '../../Graph/rendering/renderFilters';
import { renderEdges } from '../../Graph/rendering/renderEdges';
import { renderNodeShapes } from '../../Graph/rendering/renderNodes';

// Simulator-specific rendering
import { renderBasisLabelsWithOutcomes, renderPhaseLabelsSimulator, renderIdLabels } from '../rendering/renderLabels';
import { getFillColorForSimulator } from '../utils/colors'

type UseSimulatorRenderingProps = {
  mainNodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  measured: number[];
  active: number[];
  outcomes: [number, number][];
  readyToMeasure: number[];
  width: number;
  height: number;
  selectedNodes: NodeType[];
  setSelectedNodes: (nodes: NodeType[]) => void;
  onSelectionChange?: (selected: NodeType[]) => void;
  measureOperation?: (id: number) => void;
  updateOutputTables: (nodes: NodeType[], outputs: number[]) => void;
};

export const useSimulatorRendering = ({
  mainNodes,
  edges,
  inputs,
  outputs,
  measured,
  active,
  outcomes,
  readyToMeasure,
  width,
  height,
  selectedNodes,
  setSelectedNodes,
  onSelectionChange,
  measureOperation,
  updateOutputTables,
}: UseSimulatorRenderingProps) => {
  const svgRef = useRef<SVGSVGElement | null>(null);
  const nodeGroupRef = useRef<d3.Selection<SVGGElement, unknown, null, undefined> | null>(null);

  // Persisted pan offset — stored in a ref so the D3 effect always reads the
  // latest value without needing it as a dependency (which would cause resets),
  // and mirrored into state so React overlay components re-render correctly.
  const panOffsetRef = useRef<{ x: number; y: number }>({ x: 0, y: 0 });
  const [panOffset, setPanOffset] = useState<{ x: number; y: number }>({ x: 0, y: 0 });

  useEffect(() => {
    const id = requestAnimationFrame(() => {
      if (!nodeGroupRef.current) return;
      
      const correctionSetIds: number[] = selectedNodes.flatMap(
        node => node.correctionSet || []
      );
      const oddCorrectionSetIds: number[] = selectedNodes.flatMap(
        node => node.oddCorrectionSet || []
      );

      nodeGroupRef.current
        .selectAll<SVGCircleElement | SVGRectElement, NodeType>("circle, rect")
        .attr("filter", (d) => {
          const isSelected = selectedNodes.some(node => node.id === d.id);
          const isInCorrection = correctionSetIds.includes(d.id);
          const isInOddCorrection = oddCorrectionSetIds.includes(d.id);

          return isSelected && isInCorrection
            ? "url(#selectedCorrectionGlow)"
            : isSelected
            ? "url(#selectedGlow)"
            : isInCorrection
            ? "url(#correctionGlow)"
            : isInOddCorrection
            ? "url(#oddCorrectionGlow)"
            : null;
        });
    });

    return () => cancelAnimationFrame(id);
  }, [selectedNodes]);


  useEffect(() => {
    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    const defs = svg.append('defs');
    setupAllFilters(defs);

    const nodes = [...mainNodes];
    const simEdges = edges.map((e) => ({
      ...e,
      source: e.source,
      target: e.target,
    })) as unknown as SimEdge[];

    const simulation = d3
      .forceSimulation(nodes)
      .force("link", d3.forceLink(simEdges).id((d: any) => d.id));

    // Root group that everything is rendered into — translated on pan
    const rootGroup = svg.append("g").attr("class", "pan-root");

    rootGroup.attr(
      "transform",
      `translate(${panOffsetRef.current.x},${panOffsetRef.current.y})`
    );

    let isDragging = false;
    let dragStart = { x: 0, y: 0 };

    svg
      .style("cursor", "grab")
      .on("mousedown.pan", (event: MouseEvent) => {
        if (event.button !== 0) return;
        if ((event.target as Element).closest(".node-group")) return;

        isDragging = true;
        dragStart = {
          x: event.clientX - panOffsetRef.current.x,
          y: event.clientY - panOffsetRef.current.y,
        };
        svg.style("cursor", "grabbing");
        event.preventDefault();
      })
      .on("mousemove.pan", (event: MouseEvent) => {
        if (!isDragging) return;
        const next = {
          x: event.clientX - dragStart.x,
          y: event.clientY - dragStart.y,
        };
        panOffsetRef.current = next;
        rootGroup.attr("transform", `translate(${next.x},${next.y})`);
        setPanOffset({ ...next });
      })
      .on("mouseup.pan", () => {
        if (!isDragging) return;
        isDragging = false;
        svg.style("cursor", "grab");
      })
      .on("mouseleave.pan", () => {
        if (!isDragging) return;
        isDragging = false;
        svg.style("cursor", "grab");
      });

    const link = renderEdges(rootGroup, simEdges);

    const nodeGroup = rootGroup.append("g").attr("stroke", "#fff").attr("class", "node-group");
    nodeGroupRef.current = nodeGroup;

    const node = nodeGroup
      .selectAll<SVGGElement, NodeType>("g")
      .data(nodes)
      .join("g");

    node.on("click", function (_event, clicked) {
      setSelectedNodes([clicked]);
      if (onSelectionChange) onSelectionChange([clicked]);
    });

    node.on("dblclick", function (_event, clicked) {
      setSelectedNodes([clicked]);
      if (onSelectionChange) onSelectionChange([clicked]);

      if (measureOperation && readyToMeasure && readyToMeasure.includes(clicked.id)) {
        measureOperation(clicked.id);
      }
    });

    node.style("cursor", "default");


    renderNodeShapes(node, inputs, outputs, (d: NodeType) => getFillColorForSimulator(d, measured, active));

    const labelsT = renderBasisLabelsWithOutcomes(rootGroup, nodes, outcomes);
    const labelsPhase = renderPhaseLabelsSimulator(rootGroup, nodes, measured);
    const labelsId = renderIdLabels(rootGroup, nodes);

    simulation.on("tick", () => {
      link
        .attr("x1", (d) => (d.source as NodeType).x!)
        .attr("y1", (d) => (d.source as NodeType).y!)
        .attr("x2", (d) => (d.target as NodeType).x!)
        .attr("y2", (d) => (d.target as NodeType).y!);

      node.attr("transform", (d) => `translate(${d.x},${d.y})`);

      labelsT.attr("x", (d) => d.x!).attr("y", (d) => d.y!);
      labelsPhase.attr("x", (d) => d.x!).attr("y", (d) => d.y!);
      labelsId.attr("x", (d) => d.x!).attr("y", (d) => d.y!);

      updateOutputTables(nodes, outputs);
    });

    return () => {
      svg.on("mousedown.pan mousemove.pan mouseup.pan mouseleave.pan", null);
      svg.style("cursor", null);
    };

  }, [mainNodes, edges, inputs, outputs, measured, outcomes, readyToMeasure, width, height]);

  return { svgRef, panOffset };
};