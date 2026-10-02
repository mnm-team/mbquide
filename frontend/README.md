# MBQuIDE Frontend

## Help & Tutorials

Press **`?`** anywhere in the app to open a guided, step-by-step tutorial overlay covering every page below, each with a short video demo. It opens on top of whatever you're currently doing without losing your work, and the same content is also browsable as its own page at `/TUTORIAL`.

## QASM Input

Enter or paste an OpenQASM 2.0 circuit into the text area; a circuit diagram below it (plus qubit/gate/depth stat chips) updates on every keystroke, parsed entirely in the browser.

| Action | Interaction |
|--------|-------------|
| Load an example circuit | **Click** an entry under **Example Circuits** |
| Translate to an MBQC pattern | Click **MBQC Diagram** to jump to the editor with the translated graph |
| Translate and simulate | Click **Run Simulation** to translate, simplify, compute flow, and jump to the Simulator, all in one step |

## Editor

### Graph Interaction

| Action | Interaction |
|--------|-------------|
| Move the graph | **Hold `Ctrl` and drag** on the background |
| Select multiple vertices | **Drag on background** (without holding `Ctrl`) to create a selection brush |
| Select a single vertex | **Click** directly on the vertex |
| Move a vertex | **Click and drag** the vertex to a new position |
| Recenter the view | Press **`C`** |
| Undo / redo | **`Ctrl+Z`** / **`Ctrl+Y`** |

### Building Mode

Building mode allows unrestricted graph editing and construction. It can be accessed via the radio button in the top right or by pressing `B`.

#### Creating Vertices

- Drag **example nodes** from the **right screen edge** onto the graph field to create a vertex.
- To create an **input vertex**, drag the **example input node** over the desired **basis example (X, Y, XY)**.

#### Editing Vertices

| Action | Interaction |
|------|-------------|
| Remove a vertex | **Middle-click** a node, or select it and press `Del` |
| Set vertex phase | **Double-click** a node, or press `Enter` while it is selected, then enter a value |
| Add or remove an edge | **Right-click drag** between two nodes |

When leaving building mode:

- Structural editing is disabled.
- Only semantics-preserving rewriting operations remain available.


### Default Mode

In the default mode, the editor ensures that all operations preserve the computation semantics.  
Users can simplify or restructure the pattern using rewriting rules.

#### Available Operations

| Operation | How to Use |
|-----------|------------|
| 💥 **Relabeling** | Right-click a vertex that supports relabeling and select the desired new label from the menu. |
| 💥 **Local Complementation** | Select a vertex and apply **Local Complementation** |
| 💥 **Pivot** | Use the brush tool to select two adjacent vertices, then apply a **Pivot** to the edge between them. |
| 💥 **Z-Insert** | Insert a **Z vertex** connected to all currently selected vertices. |
| 💥 **Z-Delete** | Remove a **Z**, **XZ**, or **YZ** vertex. |
| 💥 **YZ-Unfusion** | Right-click an **XY** vertex and choose **YZ-Unfuse** to attach a pendant **YZ** vertex. Select either of the pair to reveal a draggable handle on the connecting edge: drag to split the angle between the two (snaps to steps of π/8), or double-click the handle to type an exact value. |
| 🧙‍♂️ **Reduce Nodes** | No selection needed — automatically removes non-input Clifford-angle measurements to minimize vertex count. |
| 🧙‍♂️ **Reduce Edges** | No selection needed — greedily applies local complementation/pivot (including on partial neighborhoods) to minimize edge count. Disabled once the graph is already edge-minimal. |

Every **OUTPUT** vertex also carries a small In/Out/Sign table tracking how the accumulated rewrites have reshaped its ideal X and Z observables (a fresh output starts at the identity: `Out X, Sign +` / `Out Z, Sign +`).

### Pauli Flow Visualization 🌊

1. Click the **🌊 Flow button**.
2. The backend computes a **maximally delayed focused Pauli flow**.
3. If a valid flow exists:
   - Vertices are arranged into **measurement layers**.
   - Correction dependencies are visualized.


When selecting a vertex **u**:

| Color | Meaning |
|------|--------|
| 🔵 Blue | Selected vertex \(u\) |
| 🟠 Orange | Vertices in \(f(u)\) |
| 🟢 Green | Odd neighbors of \(f(u)\) |

If the graph changes, the current flow becomes invalid and must be recomputed.

---

## Simulator

The simulator allows interactive execution of an MBQC pattern directly on the graph.

### Graph Interaction

| Action | Interaction |
|------|-------------|
| Move the graph | Click and drag on the **background** |
| Zoom in / out | **Scroll** (mouse wheel or trackpad gesture) |
| Show correction function | **Click** on a vertex |
| Measure a vertex | **Double-click** a vertex if it is currently measurable |

### Vertex States

During the simulation, vertices visually indicate their current status:

| Visual State | Meaning |
|--------------|--------|
| Faded vertex | The qubit has **not yet been initialized** |
| Normal vertex | The qubit is **initialized and ready** |
| Greyed-out vertex | The qubit has **already been measured** and shows its outcome |

### Reading the Statevector Panel

Each row is one basis state of the currently-active (non-faded) vertices. The bar's width is the probability $P = |\text{amplitude}|^2$; its color encodes the amplitude's phase $\varphi$, read off the legend gradient at the bottom of the panel.

### Input State

The simulator allows specifying the **input quantum state**.

1. At the top of the simulator, enter the **amplitudes** corresponding to the computational basis vectors.
2. Press **Submit** to initialize the state.

Requirements:

- The amplitudes must define a **valid quantum state**.
- The probabilities must sum to **1**.
- The state is only accepted if these conditions are satisfied.

Input states can only be set **at the beginning of the simulation**.

### Measurement Randomness

A **Random** checkbox controls if the measurement outcomes are generated randomly. When disabled measurements always return **0**.

This setting can also only be changed **at the beginning of the simulation**.

### Running and Resetting

The **Run All** button measures every currently-ready vertex in one click, and keeps going as newly-measured vertices make others ready, until nothing is left to measure. The **Reset** button discards all measurements and returns to the freshly-initialized state (same input statevector and random/fixed-outcome setting as before). Both buttons stay disabled until there's something for them to do.