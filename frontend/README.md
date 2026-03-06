# MBQuIDE Frontend

## Editor


### Building Mode

Building mode allows unrestricted graph editing and construction.

#### Creating Vertices

- Drag **example nodes** from the **right screen edge** onto the graph field to create a vertex.
- To create an **input vertex**, drag the **example input node** over the desired **basis example (X, Y, XY)**.

#### Editing Vertices

| Action | Interaction |
|------|-------------|
| Remove a vertex | **Middle-click** on the node |
| Set vertex phase | **Double-click** the node and enter the phase value |
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
| Show correction function | **Click** on a vertex |
| Measure a vertex | **Double-click** a vertex if it is currently measurable |

### Vertex States

During the simulation, vertices visually indicate their current status:

| Visual State | Meaning |
|--------------|--------|
| Faded vertex | The qubit has **not yet been initialized** |
| Normal vertex | The qubit is **initialized and ready** |
| Greyed-out vertex | The qubit has **already been measured** and shows its outcome |

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

### Resetting the Simulation

The **Reset** button restarts the simulation and restores the graph to its initial state. All previous measurement outcomes are lost.