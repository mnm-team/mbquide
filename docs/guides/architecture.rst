Architecture
============

This guide explains how the backend's pieces fit together: the request flow
through the HTTP server, the core data model, and how the build produces both
a server binary and a test suite from the same source.

Overview
--------

.. code-block:: text

   ┌───────────────┐      HTTP/JSON       ┌───────────────────────────┐
   │  Frontend     │ ───────────────────▶ │  Server (backend/src)     │
   │  (React/Vite) │ ◀─────────────────── │  Crow HTTP routes         │
   └───────────────┘   session_id cookie  └─────────────┬─────────────┘
                                                        │ links against
                                                        ▼
                                  ┌────────────────────────────┐
                                  │  backend/include + src     │
                                  │  mbquide_core (static lib) │
                                  └────────────┬───────────────┘
                                               │
            ┌───────────────────┬──────────────┼──────────────┬────────────────────┐
            ▼                   ▼              ▼              ▼                    ▼
      MBQC_Graph          ZXGraph        Flow (Pauli flow)  Simulator      QASM_Parser /
      (graph/)            (QASM/)        (graph/)          (Simulator.hpp) Quantum_Circuit

This guide focuses on the backend (the right-hand side above). For the
frontend's own structure, dev server, and how it talks to this API, see
:doc:`../frontend`.

``mbquide_core`` is a single CMake library target that both the ``Server``
executable and the ``Tests``/``Benchmarks`` executables link against, so the
same graph, flow, and simulator code is exercised in production and under
test. See :doc:`build` for how these targets are wired up.

Request Flow
-------------

1. A request arrives at one of the ``/api/*`` routes registered in
   ``backend/src/server.cpp``.
2. ``get_session_id`` reads the ``session_id`` cookie, or generates a new one
   if absent. New sessions get a ``Set-Cookie`` header on the response.
3. Session-scoped state is looked up (and lazily created) from one of four
   in-memory maps keyed by session ID: ``user_graphs``, ``user_flows``,
   ``user_simulators``, ``user_zx_graphs``. There is currently no
   persistence or eviction — state lives for the lifetime of the process.
4. The handler mutates or reads the relevant object (see :doc:`../api/rest`
   for the per-endpoint contract) and serializes the result with
   ``nlohmann::json``.

This means the server is **stateful per-session** but **not
multi-process-safe** — all session state lives in one process's memory, which
is fine for the single-user desktop-app deployment model (a pre-built binary
run locally) but would need an external session store to scale horizontally.

Core Data Model
-----------------

:cpp:class:`MBQC_Graph`
   The central data structure: a measurement-based quantum computing graph —
   vertices with a measurement basis/angle each, plus input/output vertex
   sets. Supports the graph-rewrite rules used to simplify MBQC patterns
   (local complementation, pivot, Z-insertion/deletion, relabeling) and
   serializes to/from JSON for the API layer.

:cpp:struct:`PauliFlowResult` (``Flow.hpp``)
   The result of :cpp:func:`findPauliFlow`: correction sets and measurement
   depths that certify an :cpp:class:`MBQC_Graph` can be executed
   deterministically. :cpp:func:`focus` refines a flow in place.

:cpp:class:`QuantumCircuit` / :cpp:class:`QASMParser`
   A gate-list circuit representation and its OpenQASM 2.0 parser.
   ``Circ2MBQC.hpp`` translates a parsed circuit directly into an
   :cpp:class:`MBQC_Graph` (Broadbent & Kashefi's method) — circuits are
   **not** routed through ZX-calculus to get there.

:cpp:class:`ZXGraph`
   A ZX-calculus diagram (Z/X spiders and simple/Hadamard edges). It exists for cross-checking, in tests, that a graph rewrite or conversion preserved the underlying linear map (``compareTensors()`` in
   ``backend/include/test_helpers.hpp``, via PyZX tensor contraction).

:cpp:class:`Simulator`
   Wraps one of two backends behind :cpp:class:`SimulatorBackendHandle`: a
   dense :cpp:class:`StatevectorSimulator` (Eigen-based) for small graphs, and
   a :cpp:class:`TensorNetworkSimulator` for larger ones, switching based on
   the ``maxVecSize`` threshold passed at initialization. See
   :cpp:enum:`SimulatorBackendType`.

Directory Layout
------------------

.. code-block:: text

   backend/
   ├── include/            # Public headers (the library's API surface)
   ├── src/
   │   ├── server.cpp      # Crow HTTP routes — the only file that #includes crow.h
   │   ├── utils.cpp
   │   ├── graph/           # MBQC_Graph, Flow, OutputAdjustments
   │   └── QASM/             # QASM_Parser, ZX_Graph, MBQC2ZX, ZX2MBQC
   ├── test/                # doctest-based unit tests + benchmarks
   └── CMakeLists.txt

Notice that ``server.cpp`` is the *only* translation unit that depends on
Crow — everything else in ``mbquide_core`` is HTTP-framework-agnostic, which
is what lets the test suite link the same graph/simulation code without
pulling in an HTTP server.
