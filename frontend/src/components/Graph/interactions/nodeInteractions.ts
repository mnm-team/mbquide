import * as d3 from 'd3';
import { NodeType, ContextMenuState } from '../types';

export const applyNodeInteractions = (
  node: d3.Selection<SVGGElement, NodeType, SVGGElement, unknown>,
  setSelectedNodes: (nodes: NodeType[]) => void,
  onSelectionChange?: (selected: NodeType[]) => void,
  onNodeDelete?: (node?: NodeType) => void,
  setContextMenu?: (menu: ContextMenuState) => void,
  buildingMode?: boolean,
  onNodeDoubleClick?: (node: NodeType) => void,
) => {

    let clickTimer: ReturnType<typeof setTimeout> | null = null;
    const DOUBLE_CLICK_DELAY = 400;

    node.on("click", function (event, d) {
        if (buildingMode && onNodeDoubleClick) {
            // In build mode, handle double-click detection
            if (clickTimer) {
                // Second click detected
                clearTimeout(clickTimer);
                clickTimer = null;
                onNodeDoubleClick(d);
                event.stopPropagation();
            } else {
                // First click - start timer
                clickTimer = setTimeout(() => {
                    // Timer expired - it was a single click
                    clickTimer = null;
                    setSelectedNodes([d]);
                    if (onSelectionChange) onSelectionChange([d]);
                }, DOUBLE_CLICK_DELAY);
            }
        } else {
            // Not in build mode - just handle single click
            setSelectedNodes([d]);
            if (onSelectionChange) onSelectionChange([d]);
        }
    });

    // BUILD MODE
    if (buildingMode) {

        node.on("auxclick", function (event, d) {
            if (event.button === 1) {
                event.preventDefault();
                event.stopPropagation();
                if (onNodeDelete) onNodeDelete(d);
            }
        });

        node.on("contextmenu", null);

    // NO BUILD MODE
    } else {

        node.on("auxclick", null);

        node.on("contextmenu", function (event, d) {
            event.preventDefault();
            setSelectedNodes([d]);
            if (onSelectionChange) onSelectionChange([d]);

            if (setContextMenu) {
            setContextMenu({
                visible: true,
                x: event.pageX - 25,
                y: event.pageY - 25,
                node: d,
            });
            }
        });

    }
};