# MBQuIDE Backend

## API Reference

The server runs on `http://localhost:18080` and uses cookie-based sessions to maintain state across requests. A session is automatically created on your first request and persists for 24 hours.

---

## Session Management

Sessions are handled automatically via an `HttpOnly` cookie (`session_id`). Each session maintains its own isolated state for:
- An **MBQC graph**
- A **ZX graph**
- A **Pauli flow result**
- A **simulator instance**

---

## Endpoints

### `GET /api/graph`
Returns the current MBQC graph for the session.

**Response:** JSON representation of the MBQC graph. Example:
```json
{
    "edges":[[0,1],[1,2]],
    "inputs":[0],
    "meas":[["X",""],["XY","π"],["OUTPUT",""]],
    "outAdj":{"2":{"X":["X",1],"Z":["Z",1]}},
    "outputs":[2],
    "size":3
}
```
---

### `POST /api/graph`
Performs operations on the MBQC graph. The action taken depends on the fields present in the request body.

**Content-Type:** `application/json`

#### Load a graph from JSON
```json
{ "size": 4, "inputs": [0], "outputs": [3], ... }
```
Replaces the current graph with the one described. The body should be a full graph JSON object (must contain `"size"`).

#### Convert MBQC graph → ZX graph
```json
{ "transform": true }
```
Converts the current MBQC graph into a ZX graph and returns its JSON representation.

#### Apply a graph rewrite operation
```json
{ "operation": "<op>", ... }
```

| Operation | Fields | Description |
|---|---|---|
| `"lc"` | `node: int` | Local complementation on a node |
| `"pivot"` | `u: int`, `v: int` | Pivot on edge (u, v) |
| `"z-insert"` | `ids: int[]` | Z insertion on a list of nodes |
| `"z-delete"` | `node: int` | Z deletion on a node |
| `"relabel"` | `node: int` | Relabel a node |
| `"relabel-planar"` | `node: int`, `pref-basis?: string` | Planar relabelling, with optional preferred measurement basis |

#### Compute focused Pauli flow
```json
{ "flow": "pauli" }
```
Computes the Pauli flow for the current graph. Returns the graph JSON extended with a `"flow"` field, which looks like:
```json
"flow": {
    "ok": true,
    "corrf": { "0": [1], "1": [2] },
    "depths": [2,1,0],  // index of the list = ID of the vertices
    "oddNcorrf": { "0": [2], "1": [] },
}
```



#### Initialise a simulation
```json
{ "simulate": true, "random": true, "input": "(0.707107)|00> + (0.707107)|11>" }
```
Initialises the simulator using the current graph and flow. Requires a valid Pauli flow to exist.

---

### `GET /api/qasm`
Returns the current ZX graph for the session.

---

### `POST /api/qasm`
Parses a QASM string into a ZX graph, or converts the current ZX graph to an MBQC graph.

#### Parse QASM
```json
{ "qasm": "OPENQASM 2.0;\n..." }
```
Parses the provided OpenQASM 2.0 string and stores the resulting ZX graph in the session.

#### Convert ZX graph → MBQC graph
```json
{ "transform": true }
```
Converts the current ZX graph into an MBQC graph and returns its JSON representation.

```json
{
	"edges": [[0,1,2], [1,2,1],[2,3,1]],  // 1 stands for simple wire, 2 for H wire
	"inputs": [0],
	"nodes": [["INPUT",""],["Z","π"],["X",""],["OUTPUT",""]], // index = ID
	"outputs": [3],
	"size": 4
}
```

---

### `GET /api/sim`
Returns the current state of the simulator.

---

### `POST /api/sim`
Controls the simulator.

#### Initialise
```json
{ "init": true, "random": false, "input": "|0>" }
```
Initialises the simulator from the current graph and flow. Set `"random": true` to use random measurement outcomes.

#### Measure a single node
```json
{ "measureNode": 2 }
```
Performs one measurement step on the specified node.

#### Measure all nodes
```json
{ "measureAll": true }
```
Runs the full simulation to completion.


## Run Test File:

Get a Python virtual environment called python_venv and install pyzx 0.9:
```
python -m venv python_venv && python_venv/bin/python -m pip install pyzx==0.9.0
```

Run:
```
g++ src/utils.cpp src/graph/* src/QASM/* test/test* -Iinclude -o runTest && ./runTest
```
