import { TutorialStep } from './types';
import yzUnfusionVideo from './media/yz-unfusion.webm';
import panCanvasVideo from './media/pan-canvas.webm';
import rubberbandSelectVideo from './media/rubberband-select.webm';
import dragNodesVideo from './media/drag-nodes.webm';
import buildingDragNodeVideo from './media/building-drag-node.webm';
import buildingDragInputVideo from './media/building-drag-input.webm';
import edgeCreateRemoveVideo from './media/edge-create-remove.webm';
import phaseModalVideo from './media/phase-modal.webm';
import undoRedoVideo from './media/undo-redo.webm';
import recenterVideo from './media/recenter.webm';
import relabelVideo from './media/relabel.webm';
import localComplementationVideo from './media/local-complementation.webm';
import pivotVideo from './media/pivot.webm';
import zInsertVideo from './media/z-insert.webm';
import zDeleteVideo from './media/z-delete.webm';
import reduceNodesVideo from './media/reduce-nodes.webm';
import reduceEdgesVideo from './media/reduce-edges.webm';
import getFlowVideo from './media/get-flow.webm';
import simStatevectorInputVideo from './media/sim-statevector-input.webm';
import simMeasureNodeVideo from './media/sim-measure-node.webm';
import simRunResetVideo from './media/sim-run-reset.webm';
import qasmLiveRenderVideo from './media/qasm-live-render.webm';
import qasmExampleVideo from './media/qasm-example.webm';
import qasmToMbqcVideo from './media/qasm-to-mbqc.webm';
import deleteNodesVideo from './media/delete-nodes.webm';
import correctionHalosVideo from './media/correction-halos.webm';
import outputTableVideo from './media/output-table.webm';
import simRandomToggleVideo from './media/sim-random-toggle.webm';
import simStatevectorLegendVideo from './media/sim-statevector-legend.webm';
import simActiveNodesVideo from './media/sim-active-nodes.webm';

// Add new steps here to extend the tutorial - each one renders as a self-contained
// card (media + caption). Group ordering on the page follows the order steps appear here,
// and steps are grouped into sections by their `page` field.
export const tutorialSteps: TutorialStep[] = [

  // ----------- MBQC Editor - Basics --------------------

  {
    id: 'pan-canvas',
    page: 'MBQC Editor - Basics',
    title: 'Panning the canvas',
    description:
      'Hold Ctrl and drag on empty canvas space to pan the view. A plain drag (no Ctrl) ' +
      'instead draws a rubber-band selection box, so Ctrl is what tells the editor you mean ' +
      'to move the camera rather than select nodes.',
    media: { type: 'video', src: panCanvasVideo },
  },
  {
    id: 'recenter',
    page: 'MBQC Editor - Basics',
    title: 'Recentering the view',
    description:
      'If you pan far enough that the whole graph leaves the viewport, a "Press C to ' +
      'recenter" hint fades in. Pressing C jumps the view straight to centered on the ' +
      'graph’s current bounding box.',
    media: { type: 'video', src: recenterVideo },
  },
  {
    id: 'rubberband-select',
    page: 'MBQC Editor - Basics',
    title: 'Rubber-band multi-select',
    description:
      'A plain click-drag (no Ctrl) on empty canvas space draws a selection box. Any node ' +
      'whose center falls inside the box when you release is added to the selection, shown ' +
      'by a glowing outline - useful for grabbing several nodes at once before dragging, ' +
      'deleting, or running an operation on them together.',
    media: { type: 'video', src: rubberbandSelectVideo },
  },
  {
    id: 'drag-nodes',
    page: 'MBQC Editor - Basics',
    title: 'Dragging selected nodes',
    description:
      'Drag any node to reposition it. If it’s already part of the current selection, the ' +
      'whole selection moves together as a rigid group, keeping every selected node’s ' +
      'offset from the others fixed - drag a node that isn’t selected and the selection ' +
      'collapses to just that one node instead.',
    media: { type: 'video', src: dragNodesVideo },
  },
  {
    id: 'undo-redo',
    page: 'MBQC Editor - Basics',
    title: 'Undo and redo',
    description:
      'Undo and Redo in the control panel (or Ctrl+Z / Ctrl+Y) step back and forward through ' +
      'graph-changing actions - here undoing and redoing a Pivot. History holds up to 10 ' +
      'steps.',
    media: { type: 'video', src: undoRedoVideo },
  },


  // ----------- MBQC Editor - Building Mode --------------------

  {
    id: 'building-drag-node',
    page: 'MBQC Editor - Building Mode',
    title: 'Placing a node',
    description:
      'Turn on Building Mode (toggle top-right, or press B) to make the node palette on the ' +
      'right edge interactive. Drag any basis icon onto the canvas to create a new node ' +
      'there - the palette itself always stays fixed at the edge; only a preview copy of the ' +
      'icon follows your cursor while you drag.',
    media: { type: 'video', src: buildingDragNodeVideo },
  },
  {
    id: 'building-drag-input',
    page: 'MBQC Editor - Building Mode',
    title: 'Placing an INPUT node',
    description:
      'The INPUT palette icon works in two steps: drag it onto the X, Y, or XY icon in the ' +
      'same palette first to pick which basis the new input will measure in (the preview ' +
      'recolors to confirm the pick), then continue the drag onto the canvas to place it. ' +
      'Dropping without ever hovering one of those three icons does nothing.',
    media: { type: 'video', src: buildingDragInputVideo },
  },
  {
    id: 'edge-create-remove',
    page: 'MBQC Editor - Building Mode',
    title: 'Connecting nodes',
    description:
      'In Building Mode, right-click and drag from one node to another to add an edge ' +
      'between them - a dashed preview line follows your cursor while you drag. Doing the ' +
      'same drag again between an already-connected pair removes the edge instead.',
    media: { type: 'video', src: edgeCreateRemoveVideo },
  },
  {
    id: 'phase-modal',
    page: 'MBQC Editor - Building Mode',
    title: 'Setting a phase',
    description:
      'Double-click a node in Building Mode to open its phase editor (also reachable with ' +
      'Enter when exactly one node is selected). Planar nodes (XY/XZ/YZ) take a typed ' +
      'expression like "pi/2" or "3/2" - a plain number is read as a multiple of π. X/Y/Z ' +
      'nodes instead show two buttons, 0 and π, since Pauli bases only ever take those two ' +
      'values.',
    media: { type: 'video', src: phaseModalVideo },
  },
  {
    id: 'delete-nodes',
    page: 'MBQC Editor - Building Mode',
    title: 'Deleting nodes',
    description:
      'In Building Mode, select one or more nodes and press Delete or Backspace to remove ' +
      'them along with their edges - remaining node ids shift down to stay contiguous. A ' +
      'single node can also be removed with a middle-click, without selecting it first.',
    media: { type: 'video', src: deleteNodesVideo },
  },


  // ----------- MBQC Editor - Rewrite Rules --------------------

  {
    id: 'relabel',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Relabeling a node',
    description:
      'Right-click a node (outside Building Mode) for a context menu with the node’s basis, ' +
      'phase, and correction set, plus one or two "Relabel to <basis>" actions - which ones ' +
      'show up depends on the node’s current basis (an X node offers XZ or XY, a Y node ' +
      'offers YZ or XY, and so on). Picking one switches the node to that basis in place, no ' +
      'new node added.',
    sections: [
      {
        heading: 'Related actions',
        paragraphs: [
          'A plain "Relabel" action (no target basis) also appears whenever every currently ' +
            'selected node is already on a planar basis (XY/XZ/YZ) with a phase of 0, π/2, ' +
            'π, or 3π/2 - the four points where two planar bases coincide, so relabeling is ' +
            'just a change of description rather than a change of the underlying state.',
        ],
      },
    ],
    media: { type: 'video', src: relabelVideo },
  },
  {
    id: 'local-complementation',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Local Complementation',
    description:
      'Select exactly one non-input node and click LC in the control panel. It toggles the ' +
      'edges among that node’s neighbors - present ones are removed, absent ones are added ' +
      '- and updates the node’s own basis and phase, and each neighbor’s, by a fixed rule. ' +
      'Here a hub with three otherwise-unconnected neighbors becomes a fully-connected ' +
      'group of four, since none of the neighbor-to-neighbor edges existed before.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Local complementation is the standard graph-state rewrite rule at the heart of ' +
            'the ZX/graph-state calculus: complementing the neighborhood of a vertex - ' +
            'flipping every edge between pairs of its neighbors - realizes a specific local ' +
            'Clifford operation on the underlying stabilizer state, applied entirely by ' +
            'graph surgery with no change to the state itself.',
          'The measurement basis and phase update alongside the edges so that the MBQC ' +
            'pattern the graph encodes is preserved - each of the six single-qubit bases ' +
            '(X/Y/Z and the planar XY/XZ/YZ) has a fixed rule for what it becomes on the ' +
            'complemented node and on each of its neighbors, keeping the same measurement ' +
            'outcomes to the same effect.',
          'This is also the building block behind two other operations in the editor: Pivot ' +
            'is exactly the sequence LC(u), LC(v), LC(u) for an edge (u,v), and "Reduce ' +
            'Edges" greedily searches for LC/Pivot sequences that reduce the total edge count.',
        ],
      },
    ],
    media: { type: 'video', src: localComplementationVideo },
  },
  {
    id: 'pivot',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Pivot',
    description:
      'Select exactly two adjacent nodes and click Pivot. It rewires connections around ' +
      'that edge - here, two nodes that connected into a shared hub have their connections ' +
      'hop over to the hub’s other endpoint instead, so the roles of "hub" and "leaf" swap ' +
      'between the pivoted pair.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Pivot is not a separate rewrite rule: pivoting about an edge (u, v) is exactly ' +
            'the sequence LC(u), then LC(v), then LC(u) again - the standard identity ' +
            'expressing graph-state pivoting as three local complementations. See the Local ' +
            'Complementation entry for what each of those steps does to edges and ' +
            'measurement bases.',
          'Net effect on the edge set: every neighbor exclusive to u swaps to being a ' +
            'neighbor of v instead (and vice versa), while neighbors shared by both stay ' +
            'connected to both - u and v themselves stay connected throughout.',
        ],
      },
    ],
    media: { type: 'video', src: pivotVideo },
  },
  {
    id: 'z-insert',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Z-Insert',
    description:
      'Select one or more nodes and click Z-Insert to add a fresh Z-basis node (angle 0), ' +
      'connected by an edge to every selected node. With nothing selected it drops an ' +
      'unconnected Z node onto the canvas instead - a quick way to add a building block you ' +
      'can wire up or relabel by hand afterward.',
    media: { type: 'video', src: zInsertVideo },
  },
  {
    id: 'z-delete',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Z-Delete',
    description:
      'Select one or more Z/XZ/YZ-basis nodes with phase 0 or π and click Z-Delete to ' +
      'remove them, folding their effect into their neighbors instead of just cutting them ' +
      'loose. Here, deleting a Z node at phase π shifts its XY neighbor’s phase by +π (π/2 ' +
      'becomes 3π/2) - the deleted node’s angle (0 or π) determines whether each neighbor’s ' +
      'phase shifts at all.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Z-Deletion is only well-defined for a Z/XZ/YZ-basis node whose angle is exactly ' +
            '0 or π - these are the two angles where a Z-plane measurement has no free ' +
            'phase to lose by being deleted, so no other angle can be removed this way.',
          'Deleting a node with angle a·π (a = 0 or 1) shifts each neighbor’s phase ' +
            'depending on its own basis: an X, Y, or XY neighbor\'s angle shifts by +a·π ' +
            '(a no-op when a=0); an XZ or YZ neighbor\'s angle negates when a=1 and is left ' +
            'alone when a=0; an OUTPUT neighbor instead picks up a Pauli-Z output ' +
            'adjustment when a=1.',
        ],
      },
    ],
    media: { type: 'video', src: zDeleteVideo },
  },
  {
    id: 'output-table',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Reading an OUTPUT node’s table',
    description:
      'Every OUTPUT node carries a small In/Out/Sign table tracking how graph edits have ' +
      'reshaped its ideal X and Z observables so far - a fresh output starts at the ' +
      'identity (Out X, Sign +; Out Z, Sign +). Here, Z-deleting a node wired directly to ' +
      'the output flips the X row’s sign to − while leaving the Z row untouched, since a Z ' +
      'operation anticommutes with X but commutes with itself.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'The table is a compact record of a single-qubit Pauli-frame correction: each row ' +
            'shows what the output’s ideal X (or Z) operator has become - which Pauli it now ' +
            'is (Out) and with what sign - after being conjugated by every graph operation ' +
            'applied near it so far: each operation P updates the running record via ' +
            'op·P·op⁻¹.',
          'Local Complementation contributes to this too, not just Z-Delete: an LC on the ' +
            'output itself conjugates by Rx(π/2), and an LC on one of its neighbors ' +
            'conjugates the output by Rz(−π/2) - both visible as the Out/Sign columns ' +
            'changing without the output node itself moving or changing basis.',
          'This output-correction bookkeeping is the same Pauli-frame tracking used to ' +
            'reconcile stabilizer measurement outcomes with a circuit’s intended output ' +
            '(arXiv:2103.02202 §2.3) - it keeps the pattern’s declared output equivalent to ' +
            'the original circuit\'s, regardless of which internal graph rewrites were used ' +
            'to get there.',
        ],
      },
    ],
    media: { type: 'video', src: outputTableVideo },
  },
  {
    id: 'yz-unfusion-drag',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'YZ-Unfusion angle drag',
    description:
      "Right-click an XY node and choose \"YZ-Unfuse\" to attach a draggable YZ pendant node. " +
      "Selecting either node in the pair then reveals an orange draggable handle on the " +
      "connecting edge. Dragging the handle slides the shared angle between the two nodes, " +
      "snapping to steps of π/8. Double-click the handle to type an exact angle instead.",
    media: { type: 'video', src: yzUnfusionVideo },
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'A YZ-unfusion splits a single XY-measured node with angle β into a pair: the same ' +
            'XY node plus a new YZ-measured pendant node with angle γ. The two angles satisfy ' +
            'the invariant α = β − γ (mod 2π), fixed at the moment the pendant is created and ' +
            'preserved by every later edit to the pair.',
          "Dragging the handle changes β directly, snapped to multiples of π/8. The pendant's " +
            'angle is then recomputed as γ = β − α, so the invariant always holds - the handle ' +
            'parameterizes the pair by one shared angle rather than two independent ones.',
          "The handle's position along the edge is a direct readout of β: it sits flush " +
            'against the XY node when β = 2π (shown as a blank phase label, since 2π ≡ 0) and ' +
            'slides toward the YZ node as β decreases toward 0.',
        ],
      },
      {
        heading: 'Related actions',
        paragraphs: [
          'Double-clicking the handle opens a modal for typing an exact angle instead of ' +
            'dragging, for either node in the pair.',
          'The same right-click context menu offers "Relabel", which switches a node between ' +
            'the X/Y/Z axis bases and the XY/XZ/YZ planar bases - a separate operation from ' +
            'YZ-unfusion, which instead adds a node.',
        ],
      },
    ],
  },
  {
    id: 'reduce-nodes',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Reduce Nodes (Simplify)',
    description:
      'Click Reduce Nodes to run the graph through an automatic simplification pass - no ' +
      'selection needed, it looks at the whole graph. Here a Y-basis hub between two ' +
      'quarter-angle XY leaves collapses from three nodes down to two X-basis nodes with a ' +
      'single edge between them.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Simplification repeats a fixed-point loop - each pass relabels any planar ' +
            '(XY/XZ/YZ) non-output node whose angle is a multiple of π/2 to the matching ' +
            'Pauli basis, local-complements every Y-basis node that isn’t an input or ' +
            'output, and pivots every X-basis non-input node against a non-input neighbor - ' +
            'until a pass makes no more changes.',
          'The two operations doing the actual work, Y-node local complementation and ' +
            'X-node pivoting, are exactly the LC and Pivot operations elsewhere in this menu ' +
            '- Reduce Nodes is really "keep applying LC and Pivot wherever they simplify a ' +
            'Pauli-basis node, automatically."',
        ],
      },
    ],
    media: { type: 'video', src: reduceNodesVideo },
  },
  {
    id: 'reduce-edges',
    page: 'MBQC Editor - Rewrite Rules',
    title: 'Reduce Edges (Simplify)',
    description:
      'Click Reduce Edges to greedily search for LC/Pivot sequences that reduce the total ' +
      'edge count - no selection needed. The button only enables once a reducing step is ' +
      'known to exist, so it stays disabled once the graph is already edge-minimal under ' +
      'LC/Pivot. The search can cascade further than a single step: a 4-node cycle here, ' +
      'every node measured at angle π/2, collapses all the way down to one isolated node.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'This is a greedy edge-reduction heuristic: at each step, it evaluates every ' +
            'currently-available LC or Pivot move by how many edges it would remove, ' +
            'applies whichever move scores best, and repeats until no move would reduce ' +
            'the edge count any further.',
          'Because each step is a real LC or Pivot, this is the ' +
            'same rewrite system as Reduce Nodes and the manual LC/Pivot buttons, just ' +
            'driven by "does this reduce edge count" instead of "does this simplify a ' +
            'Pauli-basis node" - the two automatic buttons can end up making different ' +
            'choices on the same graph.',
        ],
      },
    ],
    media: { type: 'video', src: reduceEdgesVideo },
  },
  
  // ----------- MBQC Editor - Flow --------------------

  {
    id: 'get-flow',
    page: 'Flow',
    title: 'Get Flow',
    description:
      'Click Flow to compute a Pauli flow for the graph - a valid measurement order and set ' +
      'of X/Z corrections that make the pattern deterministic despite random measurement ' +
      'outcomes. Success draws dashed vertical layer lines marking that order and enables ' +
      'the Simulate button; if no flow exists, the graph can’t be run as a deterministic ' +
      'pattern and the editor says so instead.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Computing the flow follows Theorem 4.4 of Mitosek & Backens, "An algebraic ' +
            'interpretation of Pauli flow" (arXiv:2410.23439), an O(n³) algorithm: it ' +
            'builds a flow-demand matrix (the paper\'s Definition 3.4) and an order-demand ' +
            'matrix (Definition 3.5) and solves for a correction function from those.',
          'A Pauli flow has three parts: a correction function mapping each node to the ' +
            'set of nodes whose X-correction its outcome propagates to; the induced ' +
            'odd-neighborhood correction, giving the Z-correction for each node; and a ' +
            'partial order over the nodes compatible with the flow, which is what the ' +
            'dashed layer lines and the left-to-right node layout both display.',
          'Pauli flow generalizes the earlier, stricter "gflow" and "flow" notions by also ' +
            'accounting for the Pauli measurement bases (X/Y/Z) that don\'t need a ' +
            'correction the way a generic XY/XZ/YZ measurement does - more graphs admit a ' +
            'Pauli flow than admit a plain flow, which is why the editor computes this ' +
            'specifically rather than the older, narrower notion.',
        ],
      },
    ],
    media: { type: 'video', src: getFlowVideo },
  },
  {
    id: 'correction-halos',
    page: 'Flow',
    title: 'Inspecting a correction set',
    description:
      'After a successful Get Flow, click any node to see which other nodes its outcome ' +
      'would correct - an orange ring around every node in its X-correction set, a green ' +
      'ring around its odd-neighborhood Z-correction set. This view is shared by the editor ' +
      'and the Simulator. Here selecting the input node rings the node between it and the ' +
      'output in orange (its X-correction) and the output itself in green (its ' +
      'odd-neighborhood Z-correction).',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'Both rings come straight from the computed Pauli flow described in the Get Flow ' +
            'entry: the orange ring is the clicked node’s X-correction set, and the green ' +
            'ring is its odd-neighborhood Z-correction set.',
          'In MBQC, measuring a node in a non-Pauli basis generally produces a random ± ' +
            'outcome; the flow’s corrections exist precisely to cancel that randomness out, ' +
            'so the pattern still computes the same thing regardless of which outcome ' +
            'occurred - this view is a direct look at which later measurements carry that ' +
            'responsibility for a given node.',
        ],
      },
    ],
    media: { type: 'video', src: correctionHalosVideo },
  },



  // ----------- Simulator --------------------

  
  {
    id: 'sim-active-nodes',
    page: 'Simulator',
    title: 'Reading active nodes and edges',
    description:
      'A node shown in full color, connected by a black edge, is already part of the live ' +
      'entangled state. A faded (translucent) node and a grey edge aren’t part of it yet - ' +
      'the simulator brings the resource state online incrementally rather than all at ' +
      'once, so only the nodes and connections within reach of the next measurement are ' +
      'live. A measured node instead turns flat grey, regardless of the live/not-yet-live distinction.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'The resource state doesn’t need to exist all at once before computation starts: ' +
            'preparing a qubit in |+⟩ and entangling two already-prepared qubits are ' +
            'operations that commute with everything happening elsewhere in the ' +
            'not-yet-entangled graph, so the state can be built incrementally - entangling ' +
            'each qubit only once it’s about to be needed - without changing the outcome of ' +
            'any measurement.',
          'This works because the entangling operation used to build a graph state is ' +
            'diagonal in the computational basis, so any two of them commute with each ' +
            'other regardless of order or timing - entangling a qubit right before it’s ' +
            'needed produces exactly the same state as entangling it at the very start.',
        ],
      },
    ],
    media: { type: 'video', src: simActiveNodesVideo },
  },
  {
    id: 'sim-statevector-input',
    page: 'Simulator',
    title: 'Custom input statevector',
    description:
      'Type an amplitude expression into each |bitstring⟩ cell at the top of the Simulator ' +
      'page - plain numbers, fractions, "sqrt(...)", "pi", and "i" are all understood, so ' +
      '"1/sqrt(2)" and "1/sqrt(2)i" both parse. The live ‖ψ‖² readout turns green once the ' +
      'amplitudes you\'ve filled in are normalized, which is also what unlocks Submit - ' +
      'unfilled cells are just treated as amplitude 0. This grid only appears for up to 4 ' +
      'input qubits; beyond that the state is too large to hand-enter this way.',
    media: { type: 'video', src: simStatevectorInputVideo },
  },
  {
    id: 'sim-statevector-legend',
    page: 'Simulator',
    title: 'Reading the Statevector panel',
    description:
      'Each row is one basis state: the bar’s width is the probability P = |amplitude|², ' +
      'and its color comes from the amplitude’s phase φ, read off the legend gradient at ' +
      'the bottom of the panel. Since nothing’s been measured yet here, the panel shows the ' +
      'full joint state of every currently-active node - entangling a single input qubit ' +
      'with one output via the graph’s edge turns a 2-amplitude input into this 4-row, ' +
      '2-qubit state, with visibly different colors for each of its four distinct phases.',
    media: { type: 'video', src: simStatevectorLegendVideo },
  },
  {
    id: 'sim-random-toggle',
    page: 'Simulator',
    title: 'Random vs. forced outcomes',
    description:
      'The Random checkbox (top-left) controls whether measurement outcomes are sampled or ' +
      'forced. Toggling it immediately re-initializes the simulator with the new setting, ' +
      'discarding any measurements so far. Here, with Random unchecked, running the same ' +
      'pattern twice - with a Reset in between - gives the same outcome both times, instead ' +
      'of a fresh coin flip each run. It’s disabled once a measurement has been made, since ' +
      'switching modes mid-run wouldn’t mean anything.',
    media: { type: 'video', src: simRandomToggleVideo },
  },
  {
    id: 'sim-measure-node',
    page: 'Simulator',
    title: 'Measuring a node',
    description:
      'Double-click a node that’s ready to measure to measure it - a single click only ' +
      'selects it. "Ready" means every node its correction depends on has already been ' +
      'measured; there’s no separate visual marker for this beyond that dependency being ' +
      'satisfied, so it isn’t something you can read directly off the node itself. Once ' +
      'measured, the node grays out and shows its outcome bit, and the statevector panel on ' +
      'the right drops it and shows the remaining active nodes.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'A node becomes measurable once its dependencies under the computed Pauli flow ' +
            'are resolved: per arXiv:2207.09368 §2.2, a pending X/Z correction from an ' +
            'earlier measurement only actually affects certain later measurement bases; ' +
            'once no pending correction would affect a given node’s basis, that node is ' +
            'safe to measure next.',
          'After an unwanted (outcome = 1) measurement, the X and Z corrections it demands ' +
            'get propagated forward to keep the overall computation deterministic despite ' +
            'the random outcome - this property is known in the literature as "strong ' +
            'uniform stepwise determinism" (arXiv:2410.23439).',
        ],
      },
    ],
    media: { type: 'video', src: simMeasureNodeVideo },
  },
  {
    id: 'sim-run-reset',
    page: 'Simulator',
    title: 'Run All and Reset',
    description:
      'Run All measures every currently-ready node in one click, and keeps going as newly ' +
      'measured nodes make others ready - here it clears an entire 2-node chain in a single ' +
      'click, leaving only the output. Reset discards all measurements and returns to the ' +
      'freshly-initialized state (same input statevector and random/fixed-outcome setting ' +
      'as before); both buttons stay disabled until there\'s something for them to do - Run ' +
      'All needs a ready node, Reset needs at least one measurement to undo.',
    media: { type: 'video', src: simRunResetVideo },
  },


  // ----------- QASM Input --------------------

  {
    id: 'qasm-live-render',
    page: 'QASM Input',
    title: 'Live circuit preview',
    description:
      'Type or paste OPENQASM 2.0 into the textarea and the circuit diagram below it updates ' +
      'on every keystroke - parsing and layout both happen entirely in the browser, with no ' +
      'network round-trip and no debounce, so there’s no lag between typing and seeing the ' +
      'diagram. The stat chips (qubits, gates, depth) and the diagram itself both come from ' +
      'that same client-side parse.',
    media: { type: 'video', src: qasmLiveRenderVideo },
  },
  {
    id: 'qasm-example',
    page: 'QASM Input',
    title: 'Loading an example circuit',
    description:
      'Click any entry under Example Circuits to drop a ready-made QASM program straight ' +
      'into the textarea, replacing whatever was there. It’s an instant, local swap - no ' +
      'network round-trip - so the diagram below updates immediately, exactly as if you\'d ' +
      'typed it yourself.',
    media: { type: 'video', src: qasmExampleVideo },
  },
  {
    id: 'qasm-to-mbqc',
    page: 'QASM Input',
    title: 'From QASM to an MBQC pattern',
    description:
      'Click MBQC Diagram to translate the current QASM into an MBQC graph, then jump ' +
      'straight to the editor to see it - a single-qubit circuit here expands into a chain ' +
      'of measurement nodes. Run Simulation does the same translation plus three more ' +
      'steps chained automatically (simplify, get flow, initialize the simulator) before ' +
      'landing on the Simulator page instead, ready to measure.',
    sections: [
      {
        heading: 'Mathematical foundation',
        paragraphs: [
          'The circuit-to-pattern translation follows Broadbent & Kashefi\'s ' +
            'measurement-based quantum computing construction (arXiv:quant-ph/0704.1736): ' +
            'each gate in the circuit is rewritten into a small fixed fragment of graph plus ' +
            'measurement angles, and the fragments are wired together in place of the ' +
            'original gates - which is why even a 3-gate single-qubit circuit expands into ' +
            'several graph nodes rather than staying as compact as the circuit was.',
        ],
      },
    ],
    media: { type: 'video', src: qasmToMbqcVideo },
  },
  
];
