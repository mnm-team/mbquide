import { useEffect, useRef, RefObject } from 'react';
import * as d3 from 'd3';
import { NodeType, Edge, SimEdge, ContextMenuState, OutputAdjustment, LayerLine } from '../types';
import { OUTPUT_TABLE, SVG_DIMENSIONS } from '../utils/constants';
import { BRUSH_COLORS, getFillColor, getFillColorZX, getLabelColor, getLabelColorZX } from '../utils/colors';
import { setupAllFilters } from '../rendering/renderFilters';
import { renderEdges } from '../rendering/renderEdges';
import { renderNodeShapes } from '../rendering/renderNodes';
import { renderBasisLabels, renderPhaseLabels } from '../rendering/renderLabels';
import { renderOutputTables } from '../rendering/renderOutputTables';
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
  onNodeDrop?: (node?: NodeType, x?: number, y?: number, isInput?: boolean) => void;
  onNodeDragStart?: () => void;
  onNodeDragEnd?: (nodes: NodeType[]) => void;
  onNodeDelete?: (nodes?: NodeType[]) => void;
  onCreateNewEdge?: (edge?: Edge) => void;
  buildingMode?: boolean;
  onNodeDoubleClick?: (node: NodeType) => void;
  ignoreExamples?: boolean;
  classicZXcolors?: boolean;
  outputAdjustments?: Record<number, OutputAdjustment>;
  flowLayerLines?: LayerLine[] | null;
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
  onNodeDragStart,
  onNodeDragEnd,
  onNodeDelete,
  onCreateNewEdge,
  buildingMode,
  onNodeDoubleClick,
  ignoreExamples,
  classicZXcolors,
  outputAdjustments,
  flowLayerLines,
}: UseGraphSimulationProps) => {
  const svgRef = useRef<SVGSVGElement | null>(null);
  const nodeGroupRef = useRef<d3.Selection<SVGGElement, unknown, null, undefined> | null>(null);
  const panGroupRef = useRef<SVGGElement | null>(null);
  const panOffsetRef = useRef({ x: 0, y: 0 });

  const setPanOffset = (offset: { x: number; y: number }) => {
    panOffsetRef.current = offset;
  };

  // Update glow filters on selected nodes
  useEffect(() => {
    const id = requestAnimationFrame(() => {
      if (!nodeGroupRef.current) return;

      const correctionSetIds: number[] = selectedNodes.flatMap(n => n.correctionSet || []);
      const oddCorrectionSetIds: number[] = selectedNodes.flatMap(n => n.oddCorrectionSet || []);

      nodeGroupRef.current
        .selectAll<SVGCircleElement | SVGRectElement, NodeType>("circle, rect")
        .attr("filter", (d) => {
          const isSelected = selectedNodes.some(n => n.id === d.id);
          const isInCorrection = correctionSetIds.includes(d.id);
          const isInOddCorrection = oddCorrectionSetIds.includes(d.id);

          if (isSelected && isInCorrection) return "url(#selectedCorrectionGlow)";
          if (isSelected)                   return "url(#selectedGlow)";
          if (isInCorrection)               return "url(#correctionGlow)";
          if (isInOddCorrection)            return "url(#oddCorrectionGlow)";
          return null;
        });
    });

    return () => cancelAnimationFrame(id);
  }, [selectedNodes]);

  // Full D3 render
  useEffect(() => {
    const svgEl = svgRef.current;
    if (!svgEl) return;

    const svg = d3.select(svgEl);
    svg.selectAll('*').remove();
    svg.on("contextmenu", event => event.preventDefault());

    // Pan layer
    const panGroup = svg.insert("g", ":first-child").attr("class", "pan-layer");
    panGroupRef.current = panGroup.node();
    panGroup.attr('transform', `translate(${panOffsetRef.current.x}, ${panOffsetRef.current.y})`);
    panGroup.style("pointer-events", "all");

    // Brush layer
    const brushLayer = panGroup.insert("g", ":first-child").attr("class", "brush");
    
    const brush = createBrushBehavior(
      mainNodes,
      setSelectedNodes,
      onSelectionChange,
    );

    brushLayer
      .call(brush)
      .select(".selection")
      .attr("fill", BRUSH_COLORS.FILL)
      .attr("stroke", BRUSH_COLORS.STROKE);

    const brushOverlay = brushLayer.select<SVGRectElement>(".overlay");
    brushOverlay.on("contextmenu", (event) => event.preventDefault());
    brushOverlay.style("pointer-events", "all");

    // Key Interactions
    const handleKeyDown = (e: KeyboardEvent) => {
      const tag = (e.target as HTMLElement).tagName;
      if (tag === 'INPUT' || tag === 'TEXTAREA') return;

      if (e.key === "Control") {
        brushOverlay.style("cursor", "grab");
      }
      
      if (e.key === "Delete" || e.key === "Backspace") {
        if (!buildingMode) return;
        const selected = selectedNodesRef.current;

        if (selected.length > 0 && onNodeDelete) {
          onNodeDelete(selected);
        }
      }
    };
    const handleKeyUp = (e: KeyboardEvent) => {
      if (e.key === "Control") {
        brushOverlay.style("cursor", "crosshair");
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);

    // Filters
    const defs = panGroup.append('defs');
    setupAllFilters(defs);

    // Flow layer separator lines (dashed), drawn behind edges and nodes.
    if (flowLayerLines && flowLayerLines.length > 0) {
      panGroup
        .append('g')
        .attr('class', 'flow-layer-lines')
        .style('pointer-events', 'none')
        .selectAll('line')
        .data(flowLayerLines)
        .join('line')
        .attr('x1', d => d.x)
        .attr('x2', d => d.x)
        .attr('y1', d => d.y1)
        .attr('y2', d => d.y2)
        .attr('stroke', '#888')
        .attr('stroke-width', 1.5)
        .attr('stroke-dasharray', '8,6')
        .attr('opacity', 0.5);
    }

    // Simulation data
    const simEdges = edges.map(e => ({ ...e, source: e.source, target: e.target })) as unknown as SimEdge[];

    const simulation = d3
      .forceSimulation(mainNodes)
      .force("link", d3.forceLink(simEdges).id((d: any) => d.id));

    // Edges
    const link = renderEdges(panGroup, simEdges);


    // Main node group
    const nodeGroup = panGroup.append("g").attr("stroke", "#fff");
    nodeGroupRef.current = nodeGroup;

    const node = nodeGroup
      .selectAll<SVGGElement, NodeType>("g")
      .data(mainNodes)
      .join("g")
      .attr("class", "node")
      .call(createNodeDragBehavior(simulation, selectedNodesRef, setSelectedNodes, onSelectionChange, onNodeDragEnd, onNodeDragStart) as any);

    applyNodeInteractions(
      node,
      setSelectedNodes,
      onSelectionChange,
      onNodeDelete,
      setContextMenu,
      buildingMode,
      onNodeDoubleClick,
    );

    renderNodeShapes(node, inputs, outputs, classicZXcolors ? getFillColorZX : getFillColor);

    // Labels & output tables
    const labelsT = renderBasisLabels(panGroup, mainNodes, classicZXcolors ? getLabelColorZX : getLabelColor);
    const labelsPhase = renderPhaseLabels(panGroup, mainNodes);
    const outputTableGroups = renderOutputTables(panGroup, mainNodes, outputs, outputAdjustments ?? {});

    // Example nodes
    // Appended directly to svg (not panGroup) so they stay fixed on screen.
    const exampleNodes = createExampleNodes();
    const exampleGroup = svg.append("g").attr("class", "examples");

    const example = exampleGroup
      .selectAll<SVGGElement, NodeType>("g")
      .data(exampleNodes)
      .join("g")
      .attr("class", d => `example-${d.id}`)
      .attr("transform", d => `translate(${d.x},${d.y})`);

    if (!ignoreExamples) {
      renderExampleNodeShapes(example);
      renderExampleLabels(exampleGroup, exampleNodes);
    }

    // Attach example drag
    if (buildingMode) {
      example
        .call(createExampleDragBehavior(panGroup, example, onNodeDrop, () => panOffsetRef.current))
        .style("cursor", "grab");
    }

    // Edge creation drag
    const edgePreviewLayer = panGroup
      .append<SVGGElement>("g")
      .attr("class", "edge-preview-layer")
      .attr("pointer-events", "all");

    if (buildingMode) {
      svg.call(createEdgeDragBehavior(panGroup, edgePreviewLayer, onCreateNewEdge) as any);
    }


    // Simulation tick
    simulation.on("tick", () => {
      link
        .attr("x1", d => (d.source as NodeType).x!)
        .attr("y1", d => (d.source as NodeType).y!)
        .attr("x2", d => (d.target as NodeType).x!)
        .attr("y2", d => (d.target as NodeType).y!);

      node.attr("transform", d => `translate(${d.x},${d.y})`);

      labelsT.attr("x", d => d.x!).attr("y", d => d.y!);
      labelsPhase.attr("x", d => d.x!).attr("y", d => d.y!);

      outputTableGroups.attr(
        'transform',
        d => `translate(${(d.x ?? 0) + OUTPUT_TABLE.X_OFFSET}, ${(d.y ?? 0) + OUTPUT_TABLE.Y_OFFSET})`,
      );
    });

    // Cleanup
    return () => {
      simulation.stop();
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
    };

  }, [mainNodes, edges, inputs, outputs, onNodeDrop, contextMenu?.visible, buildingMode, flowLayerLines]);

  return { svgRef, nodeGroupRef, panGroupRef, setPanOffset};
};