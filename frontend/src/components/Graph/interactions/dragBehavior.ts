import * as d3 from 'd3';
import { NodeType } from '../types';

export const createNodeDragBehavior = (
  simulation: d3.Simulation<NodeType, undefined>,
  selectedNodesRef: React.RefObject<NodeType[]>,
  setSelectedNodes: (nodes: NodeType[]) => void,
  onSelectionChange?: (selected: NodeType[]) => void
) => {
  let dragStartPositions: Map<NodeType, { x: number; y: number }> | null = null;
  let nodesToDrag: NodeType[] = [];
  let newSelection: boolean;

  return d3
    .drag<SVGGElement, NodeType>()
    .filter(event => event.button === 0) // LEFT CLICK ONLY
    .on("start", (_event, d) => {
      if (!_event.active) simulation.alphaTarget(0.7).restart();
      
      const currentSelection = selectedNodesRef.current;
      const isInSelection = currentSelection.some(n => n.id === d.id);
      
      if (!isInSelection) {
        setSelectedNodes([d]);
        nodesToDrag = [d];
        newSelection = true;
      } else {
        nodesToDrag = [...currentSelection];
      }
      
      dragStartPositions = new Map();
      nodesToDrag.forEach(node => {
        dragStartPositions!.set(node, { x: node.x!, y: node.y! });
      });
    })
    .on("drag", (event, d) => {
      if (!dragStartPositions || nodesToDrag.length === 0) return;
      
      const startPos = dragStartPositions.get(d);
      if (!startPos) return;
      
      const dx = event.x - startPos.x;
      const dy = event.y - startPos.y;
      
      nodesToDrag.forEach(node => {
        const nodeStartPos = dragStartPositions!.get(node);
        if (nodeStartPos) {
          node.fx = nodeStartPos.x + dx;
          node.fy = nodeStartPos.y + dy;
        }
      });
    })
    .on("end", (_event, d) => {
      dragStartPositions = null;
      if (newSelection) {
        setSelectedNodes([d]);
        if (onSelectionChange) onSelectionChange([d]);
      }
      nodesToDrag = [];
    });
};