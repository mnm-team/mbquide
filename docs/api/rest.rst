REST API Reference
====================

All endpoints are served from the same process as the compiled frontend
(``backend/src/server.cpp``), listening on port ``18080`` by default. Requests
are scoped to a browser session via an HttpOnly ``session_id`` cookie, set
automatically on the first response. CORS is restricted to
``http://localhost:5173`` (the frontend dev server) plus the wildcard used by
the graph endpoint's preflight response.

.. contents::
   :local:
   :depth: 1

``GET /api/graph``
---------------------

Returns the current MBQC graph for this session as JSON, initializing a new
session with a small 4-node demo graph if none exists yet.

.. code-block:: text

   200 OK
   Content-Type: application/json

``POST /api/graph``
----------------------

The workhorse endpoint: the request body is inspected for one of several
top-level keys, and the corresponding action is taken. Exactly one key should
be present per request.

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Body key
     - Effect
     - Example body
   * - ``size``
     - Replaces the session graph, parsed via ``MBQC_Graph::fromJson``.
     - ``{"size": 4, "inputs": [0], "outputs": [3], ...}``
   * - ``transform``
     - Converts the MBQC graph to a ZX-graph and returns it instead.
     - ``{"transform": true}``
   * - ``operation``
     - Applies a graph rewrite rule (see table below) and returns the updated graph.
     - ``{"operation": "lc", "node": 1}``
   * - ``simplify``
     - Runs ``MBQC_Graph::simplify()`` (repeated local complementation/pivot reduction).
     - ``{"simplify": true}``
   * - ``optimizeEdges``
     - Runs ``MBQC_Graph::greedyOptimizeEdges()``.
     - ``{"optimizeEdges": true}``
   * - ``checkOptimizeEdges``
     - Read-only: reports whether ``greedyOptimizeEdges()`` would change anything.
     - ``{"checkOptimizeEdges": true}``
   * - ``flow``
     - Computes (``"pauli"``) or focuses (``"focus"``) the Pauli flow for the session graph.
     - ``{"flow": "pauli"}``
   * - ``simulate``
     - Initializes a simulator for the session graph/flow.
     - ``{"simulate": true, "random": true, "input": "0"}``

Supported ``operation`` values:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Operation
     - Parameters
   * - ``lc``
     - ``node`` — local complementation about a vertex.
   * - ``pivot``
     - ``u``, ``v`` — pivot on an edge.
   * - ``z-insert``
     - ``ids`` — insert Z-deletion-inverse (Y) vertices.
   * - ``z-delete``
     - ``ids`` — delete Z vertices.
   * - ``relabel``
     - ``node`` — relabel a vertex's measurement basis.
   * - ``relabel-planar``
     - ``node``, optional ``pref-basis`` — planar relabeling with a preferred basis.
   * - ``yz-unfusion``
     - ``node``, ``beta`` — YZ-plane unfusion.

Errors during parsing or graph mutation return ``500`` with a plain-text
``Error: <message>`` body.

``GET /api/qasm``
--------------------

Returns the session's current graph as JSON (same shape as ``GET
/api/graph``); useful for polling after a QASM import.

``POST /api/qasm``
---------------------

Parses an OpenQASM 2.0 program and replaces the session graph with the result
of translating it to MBQC form.

.. code-block:: text

   POST /api/qasm
   Content-Type: application/json

   {"qasm": "OPENQASM 2.0;\ninclude \"qelib1.inc\";\nqreg q[1];\nh q[0];"}

On success: ``200`` with the resulting graph JSON. On a parse error: ``400``
with ``{"status": "error", "message": "..."}``. A missing ``qasm`` field is
treated as a parse error.

``GET /api/sim``
-------------------

Returns the current simulator state for this session as JSON (empty/default
if none has been initialized yet).

``POST /api/sim``
--------------------

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Body key
     - Effect
     - Example body
   * - ``init``
     - (Re)initializes the simulator from the session's graph and Pauli flow.
     - ``{"init": true, "random": false, "input": "01"}``
   * - ``measureNode``
     - Steps the simulation by measuring one node.
     - ``{"measureNode": 2}``
   * - ``measureAll``
     - Runs the simulation to completion.
     - ``{"measureAll": true}``

``init`` accepts an optional ``maxVecSize`` to bound the statevector backend
before it switches to a tensor-network backend — see the ``SimulatorBackendType``
entry in the :doc:`cpp`.

Static Frontend Routes
--------------------------

``GET /`` and ``GET /<path>`` serve the built frontend from
``./frontend/dist``, falling back to ``index.html`` for unknown paths (SPA
client-side routing) and returning ``404`` if the built frontend is missing
entirely.
