Frontend
========

The MBQuIDE frontend is a single-page web app that renders the MBQC/ZX graph
editor and simulator UI, talking to the :doc:`backend REST API <api/rest>`.
This page covers the tech stack and project layout for developers, and the
editor/simulator interactions for users — adapted and expanded from
``frontend/README.md``.

Tech Stack
------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Library
     - Role
   * - React 19 + TypeScript
     - Component framework and static typing.
   * - Vite 6
     - Dev server and production bundler.
   * - React Router 7
     - Client-side routing between the editor, simulator, and tutorial apps.
   * - D3.js 7
     - Force layout and SVG rendering for the graph canvas.
   * - Tailwind CSS 4
     - Utility-first styling, via the ``@tailwindcss/vite`` plugin.

Project Structure
--------------------

.. code-block:: text

   frontend/
   ├── index.html
   ├── vite.config.ts        # React + Tailwind Vite plugins, no other config
   ├── tsconfig.json
   ├── public/
   └── src/
       ├── main.tsx           # Router setup — see Routes below
       ├── home.tsx           # Landing page ("/")
       ├── apps/
       │   ├── MBQC/           # Main graph editor app ("/MBQC")
       │   │   ├── index.tsx
       │   │   ├── types.ts
       │   │   └── api/graphApi.ts   # fetch() wrappers for /api/graph
       │   ├── ZX_App.tsx      # ZX-calculus view ("/ZX")
       │   ├── QASM_Input_App.tsx  # OpenQASM import ("/QASM")
       │   ├── Simulator_App.tsx   # Step-through simulator ("/SIM")
       │   └── Tutorial/        # In-app help/tutorial overlay ("/TUTORIAL")
       ├── components/
       │   ├── Graph/            # D3-based graph canvas (shared by MBQC/ZX apps)
       │   ├── Simulator/         # Simulator-specific UI (state input, statevector view)
       │   ├── ControlPanel.tsx, Buttons.tsx, Icons.tsx, Tooltip.tsx, ...
       │   └── QuantumCircuitViewer.tsx
       ├── styles/
       └── assets/

Routes
--------

Registered in ``src/main.tsx``:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Path
     - App
   * - ``/``
     - Landing page (``home.tsx``).
   * - ``/MBQC``
     - The main MBQC graph editor.
   * - ``/QASM``
     - OpenQASM import screen.
   * - ``/ZX``
     - ZX-calculus diagram view.
   * - ``/SIM``
     - Interactive step-through simulator.
   * - ``/TUTORIAL``, ``/TUTORIAL/:id``
     - In-app tutorial/help overlay, also reachable from any page via the
       floating help button (``TutorialHelpButton``).

Development
-------------

.. code-block:: bash

   cd frontend
   npm install

   npm run dev       # Vite dev server at http://localhost:5173, with HMR
   npm run build      # tsc -b && vite build -> frontend/dist/
   npm run preview    # Serve the built frontend/dist/ locally for a final check
   npm run lint       # eslint ., using eslint.config.js (flat config)

``npm run build`` is what produces the ``frontend/dist/`` directory that the
compiled ``Server`` binary serves as static files in production — see
:doc:`guides/architecture`.

.. note::
   ``npm run lint`` is clean of errors. It still reports a handful of
   ``react-hooks/exhaustive-deps`` warnings — missing ``useEffect``/
   ``useCallback`` dependencies — left as-is deliberately: blindly adding the
   listed dependencies risks introducing render loops or changed behavior in
   these particular effects, so each needs a case-by-case look rather than a
   mechanical fix.

Connecting to the Backend
----------------------------

Unlike most Vite setups, there is **no dev-server proxy** configured in
``vite.config.ts``. Instead, the frontend calls the backend directly at a
hardcoded ``http://localhost:18080`` (see ``src/apps/MBQC/api/graphApi.ts``
and the ``fetch(...)`` calls in ``ZX_App.tsx``/``Simulator_App.tsx``), relying
on the backend's CORS headers — which are themselves hardcoded to allow only
``http://localhost:5173`` (see the ``CORS`` middleware in
``backend/src/server.cpp``).

Practical implications:

* Running the backend on a different port, or the frontend dev server on a
  different port than 5173, breaks API calls until both hardcoded values are
  updated and the backend is rebuilt.
* There is no environment-variable-based API base URL — changing the target
  backend means editing source and rebuilding, not setting ``.env``.
* Session state (the MBQC graph, flow, simulator) is tied to the
  ``session_id`` cookie the backend sets — see :doc:`api/rest` for the
  cookie/session contract each endpoint relies on.

Generating Tutorial Clips with Playwright
--------------------------------------------

``playwright`` is a dev dependency, but it isn't used for automated
testing — there is no Playwright test suite in the repository. Instead, it
drives the real app to record the short looping video clips shown on the
in-app ``/TUTORIAL`` help page (``src/apps/Tutorial/scripts/``): each script
seeds a graph via the backend API, performs the actual UI interaction
(mouse/keyboard), and records the result as a ``.webm`` clip referenced from
``src/apps/Tutorial/data.ts``. Nothing on the tutorial page is hand
screen-captured, so a clip can be regenerated any time the UI changes.

One-time setup, from ``frontend/``:

.. code-block:: bash

   npm install -D playwright
   npx playwright install chromium

To (re-)record a clip, start both servers (``./backend/build/Server`` and
``cd frontend && npm run dev``), then run the corresponding script:

.. code-block:: bash

   node src/apps/Tutorial/scripts/record-yz-unfusion.js

This launches a headless Chromium, drives the app, and overwrites the
matching file under ``src/apps/Tutorial/media/``. See
``src/apps/Tutorial/scripts/README.md`` for how to add a new recording
(copying an existing script, the shared helpers in ``scripts/lib/``, and a
number of Playwright/SVG timing gotchas the existing scripts already work
around).

Using the Editor
-------------------

The rest of this page is a user-facing guide to the MBQC graph editor and
simulator, for anyone running the app rather than developing it. The in-app
``/TUTORIAL`` page (reachable from any screen via the floating help button)
covers the same ground interactively, with the recorded clips described
above — use it alongside this guide, or instead of it, for a walkthrough
inside the app itself.

Graph Interaction
~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Action
     - Interaction
   * - Move the graph
     - Hold **Ctrl** and drag on the background.
   * - Select multiple vertices
     - Drag on the background (without **Ctrl**) to create a selection brush.
   * - Select a single vertex
     - Click directly on the vertex.
   * - Move a vertex
     - Click and drag the vertex to a new position.

Building Mode
~~~~~~~~~~~~~~~

Building mode allows unrestricted graph editing and construction. Access it
via the radio button in the top right, or by pressing **B**.

**Creating vertices**

* Drag an **example node** from the right screen edge onto the graph field to
  create a vertex.
* To create an **input vertex**, drag the example input node over the desired
  basis example (X, Y, XY).

**Editing vertices**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Action
     - Interaction
   * - Remove a vertex
     - Middle-click a node, or select it and press **Del**.
   * - Set vertex phase
     - Double-click a node, or press **Enter** while it is selected, then
       enter a value.
   * - Add or remove an edge
     - Right-click and drag between two nodes.

When leaving building mode, structural editing is disabled — only
semantics-preserving rewriting operations remain available.

Default Mode
~~~~~~~~~~~~~~

In the default mode, every operation preserves computation semantics. Users
simplify or restructure the pattern using rewriting rules:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Operation
     - How to use it
   * - Relabeling
     - Right-click a vertex that supports relabeling and select the desired
       new label from the menu.
   * - Local Complementation
     - Select a vertex and apply Local Complementation.
   * - Pivot
     - Use the brush tool to select two adjacent vertices, then apply Pivot
       to the edge between them.
   * - Z-Insert
     - Insert a Z vertex connected to all currently selected vertices.
   * - Z-Delete
     - Remove a Z, XZ, or YZ vertex.
   * - Simplify
     - Automatically applies multiple rewriting rules to simplify the graph
       (the ``simplify`` endpoint — see :doc:`api/rest`).

These correspond directly to the ``operation``/``simplify``/``optimizeEdges``
request bodies documented for ``POST /api/graph``.

Pauli Flow Visualization
~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Click the **Flow** button.
2. The backend computes a maximally delayed, focused Pauli flow
   (:cpp:func:`findPauliFlow` / :cpp:func:`focus`).
3. If a valid flow exists, vertices are arranged into measurement layers and
   correction dependencies are visualized.

When selecting a vertex ``u``:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Color
     - Meaning
   * - Blue
     - The selected vertex ``u``.
   * - Orange
     - Vertices in the correction set ``f(u)``.
   * - Green
     - Odd neighbors of ``f(u)``.

If the graph changes, the current flow becomes invalid and must be
recomputed.

Simulator
-----------

The simulator allows interactive execution of an MBQC pattern directly on the
graph.

Graph Interaction
~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Action
     - Interaction
   * - Move the graph
     - Click and drag on the background.
   * - Zoom in / out
     - Scroll (mouse wheel or trackpad gesture).
   * - Show correction function
     - Click on a vertex.
   * - Measure a vertex
     - Double-click a vertex, if it is currently measurable.

Vertex States
~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Visual state
     - Meaning
   * - Faded vertex
     - The qubit has not yet been initialized.
   * - Normal vertex
     - The qubit is initialized and ready.
   * - Greyed-out vertex
     - The qubit has already been measured and shows its outcome.

Input State
~~~~~~~~~~~~~

At the top of the simulator, enter the amplitudes corresponding to the
computational basis vectors, then press **Submit** to initialize the state.
The amplitudes must define a valid quantum state (probabilities summing to
1) — the state is only accepted if this holds. Input states can only be set
at the beginning of the simulation (this corresponds to the ``init`` request
body for ``POST /api/sim`` — see :doc:`api/rest`).

Measurement Randomness
~~~~~~~~~~~~~~~~~~~~~~~~

A **Random** checkbox controls whether measurement outcomes are generated
randomly; when disabled, measurements always return 0. This can also only be
changed at the beginning of the simulation.

Resetting the Simulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Reset** button restarts the simulation and restores the graph to its
initial state. All previous measurement outcomes are lost.
