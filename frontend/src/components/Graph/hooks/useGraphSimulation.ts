import { useEffect, useRef, RefObject } from 'react';
import * as d3 from 'd3';
import { NodeType, Edge, SimEdge, ContextMenuState } from '../types';
import { SVG_DIMENSIONS } from '../utils/constants';
import { BRUSH_COLORS, getFillColor, getFillColorZX, getLabelColor, getLabelColorZX } from '../utils/colors';
import { setupAllFilters } from '../rendering/renderFilters';
import { renderEdges } from '../rendering/renderEdges';
import { renderNodeShapes } from '../rendering/renderNodes';
import { renderBasisLabels, renderPhaseLabels } from '../rendering/renderLabels';
import { createExampleNodes, renderExampleNodeShapes, renderExampleLabels } from '../rendering/renderExamples';
import { createNodeDragBehavior } from '../interactions/dragBehavior';
import { createExampleDragBehavior } from '../interactions/exampleDrag';
import { createBrushBehavior } from '../interactions/brushBehavior';
import { createEdgeDragBehavior } from '../interactions/edgeCreation';
import { applyNodeInteractions } from '../interactions/nodeInteractions';

type UseGraphSimulationProps = {
  mainNodes: NodeType[];
  edges: Edge[];
  inputs: number[];
  outputs: number[];
  selectedNodes: NodeType[];
  selectedNodesRef: RefObject<NodeType[]>;
  setSelectedNodes: (nodes: NodeType[]) => void;
  setContextMenu?: (menu: ContextMenuState) => void;
  contextMenu?: ContextMenuState;
  onSelectionChange?: (selected: NodeType[]) => void;
  onNodeDrop?: (node?: NodeType) => void;
  onNodeDelete?: (node?: NodeType) => void;
  onCreateNewEdge?: (edge?: Edge) => void;
  updateOutputTables?: (nodes: NodeType[], outputs: number[]) => void;
  buildingMode?: boolean;
  onNodeDoubleClick?: (node: NodeType) => void;
  ignoreExamples?: boolean;
  classicZXcolors?: boolean;
};

export const useGraphSimulation = ({
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
  updateOutputTables,
  buildingMode,
  onNodeDoubleClick,
  ignoreExamples,
  classicZXcolors,
}: UseGraphSimulationProps) => {
  const svgRef = useRef<SVGSVGElement | null>(null);
  const nodeGroupRef = useRef<d3.Selection<SVGGElement, unknown, null, undefined> | null>(null);

  // Update node glow effects based on selection
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

  // Main D3 rendering effect
  useEffect(() => {
    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    svg.on("contextmenu", event => event.preventDefault());  // Prevent right click on svg

    const { WIDTH, HEIGHT } = SVG_DIMENSIONS;

    // Setup filters
    const defs = svg.append('defs');
    setupAllFilters(defs);

    // Prepare data
    const nodes = [...mainNodes];
    const simEdges = edges.map((e) => ({
      ...e,
      source: e.source,
      target: e.target,
    })) as unknown as SimEdge[];

    // Create simulation
    const simulation = d3
      .forceSimulation(nodes)
      .force("link", d3.forceLink(simEdges).id((d: any) => d.id));

    // Render edges
    const link = renderEdges(svg, simEdges);

    // Render main nodes
    const brushLayer = svg.append("g").attr("class", "brush");
    const nodeGroup = svg.append("g").attr("stroke", "#fff");
    nodeGroupRef.current = nodeGroup;

    const node = nodeGroup
      .selectAll<SVGGElement, NodeType>("g")
      .data(nodes)
      .join("g")
      .attr("class", "node")
      .call(createNodeDragBehavior(simulation, selectedNodesRef, setSelectedNodes, onSelectionChange) as any)
    
    applyNodeInteractions(
      node,
      setSelectedNodes,
      onSelectionChange,
      onNodeDelete,
      setContextMenu,
      buildingMode,
      onNodeDoubleClick,
    );

    renderNodeShapes(node, inputs, outputs, classicZXcolors ? getFillColorZX : getFillColor );

    // Render labels
    const labelsT = renderBasisLabels(svg, nodes, classicZXcolors ? getLabelColorZX : getLabelColor);
    const labelsPhase = renderPhaseLabels(svg, nodes);

    // Render example nodes
    const exampleNodes = createExampleNodes();
    const exampleGroup = svg.append("g").attr("class", "examples");

    const example = exampleGroup
      .selectAll<SVGGElement, NodeType>("g")
      .data(exampleNodes)
      .join("g")
      .attr("class", (d) => `example-${d.id}`)
      .attr("transform", (d) => `translate(${d.x},${d.y})`);
      
    if (buildingMode) {
      example
        .call(createExampleDragBehavior(svg, example, onNodeDrop))
        .style("cursor", "grab");
    }

    if (!ignoreExamples) {
      renderExampleNodeShapes(example);
      renderExampleLabels(exampleGroup, exampleNodes);
    }

    // Setup new Edge creation
    const edgePreviewLayer = svg
      .append<SVGGElement>("g")
      .attr("class", "edge-preview-layer")
      .attr("pointer-events", "all");

    if (buildingMode) {
      svg.call(createEdgeDragBehavior(svg, edgePreviewLayer, onCreateNewEdge) as any);
    }

    // Setup brush
    const brush = createBrushBehavior(WIDTH, HEIGHT, mainNodes, setSelectedNodes, onSelectionChange);
    brushLayer
      .call(brush)
      .select(".selection")
      .attr("fill", BRUSH_COLORS.FILL)
      .attr("stroke", BRUSH_COLORS.STROKE);

    // Simulation tick
    simulation.on("tick", () => {
      link
        .attr("x1", (d) => (d.source as NodeType).x!)
        .attr("y1", (d) => (d.source as NodeType).y!)
        .attr("x2", (d) => (d.target as NodeType).x!)
        .attr("y2", (d) => (d.target as NodeType).y!);

      node.attr("transform", (d) => `translate(${d.x},${d.y})`);
      labelsT.attr("x", (d) => d.x!).attr("y", (d) => d.y!);
      labelsPhase.attr("x", (d) => d.x!).attr("y", (d) => d.y!);
      
      if (updateOutputTables) updateOutputTables(nodes, outputs);
    });

  }, [mainNodes, edges, inputs, outputs, onNodeDrop, contextMenu?.visible]);

  return { svgRef, nodeGroupRef };
};