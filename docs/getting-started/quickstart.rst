Quickstart
==========

There are two ways to get MBQuIDE running — pick whichever matches what
you're doing:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * -
     - Option 1: Pre-built Binary
     - Option 2: From Source
   * - Best for
     - Just using the editor
     - Backend/frontend development
   * - Requires
     - Nothing — no compiler, no Node.js
     - Everything in :doc:`installation`'s *Building From Source* section
   * - Time
     - About a minute
     - A few minutes for the first build

Option 1: Run a Pre-built Binary
------------------------------------

The fastest way to run MBQuIDE: download a release archive for your platform
and run it directly. Nothing to compile, no dependencies to install.

Linux (Ubuntu 24+)
~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   mkdir -p MBQuIDE
   cd MBQuIDE
   curl -L https://github.com/mnm-team/mbquide/releases/latest/download/mbquide-v0.1.0-linux-ubuntu24.tar.gz | tar -xz
   chmod +x Server launch.sh
   ./launch.sh

Linux (Ubuntu 22)
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   mkdir -p MBQuIDE
   cd MBQuIDE
   curl -L https://github.com/mnm-team/mbquide/releases/latest/download/mbquide-v0.1.0-linux-ubuntu22.tar.gz | tar -xz
   chmod +x Server launch.sh
   ./launch.sh

macOS
~~~~~~~

.. code-block:: bash

   mkdir -p MBQuIDE
   cd MBQuIDE
   curl -L https://github.com/mnm-team/mbquide/releases/latest/download/mbquide-v0.1.0-macos.tar.gz | tar -xz
   chmod +x Server launch.sh
   ./launch.sh

Windows
~~~~~~~~~

1. Download the `latest Windows release
   <https://github.com/mnm-team/mbquide/releases/latest>`_
2. Extract the zip
3. Open the extracted folder and double-click **launch.bat**

Or via Command Prompt:

.. code-block:: bat

   curl -L -o mbquide.zip https://github.com/mnm-team/mbquide/releases/latest/download/mbquide-v0.1.0-windows.zip
   tar -xf mbquide.zip
   cd mbquide-v0.1.0-windows
   launch.bat

Once running — any platform — open your browser at:

.. code-block:: text

   http://localhost:18080

That's the whole app: ``launch.sh``/``launch.bat`` starts the compiled
``Server`` directly, which serves the already-built frontend as static files
itself. There's no separate frontend dev server to start and no source code
involved — see :doc:`../guides/architecture` if you're curious how that
works under the hood.

Releases are published for Linux (Ubuntu 22 and 24), macOS, and Windows; see
the `releases page <https://github.com/mnm-team/mbquide/releases/latest>`_
for the full list of archives.

.. note::
   This path is for *using* the editor, not developing it. If you want to
   modify the backend or frontend source, use Option 2 below instead.

Option 2: Run From Source
------------------------------

For backend/frontend development, or if no pre-built release matches your
platform. This assumes you've already followed :doc:`installation`'s
*Building From Source* section (CMake configured, backend built, frontend
dependencies installed).

Start Both Servers
~~~~~~~~~~~~~~~~~~~~~

The easiest way to run everything is the repository's helper script, which
starts the C++ backend and the Vite frontend dev server together:

.. code-block:: bash

   bash start.sh

.. list-table::
   :header-rows: 1
   :widths: 25 35

   * - Component
     - URL
   * - Backend API
     - ``http://localhost:18080``
   * - Frontend (dev)
     - ``http://localhost:5173``

Open ``http://localhost:5173`` in your browser to use the editor. During
development, the frontend dev server proxies API requests to the backend, so
edits to either side are picked up live.

Running the Backend Alone
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you only need the REST API (e.g. you're testing it with ``curl`` or
building a different frontend against it), run the compiled server directly:

.. code-block:: bash

   ./backend/build/Server

By default it listens on port ``18080`` and serves the built frontend from
``./frontend/dist`` if present, falling back to a 404 for unknown static
paths otherwise.

Talk to the API
~~~~~~~~~~~~~~~~~~

Every session is tracked by a ``session_id`` cookie that the server sets on
the first request. ``curl`` needs a cookie jar to keep it:

.. code-block:: bash

   # Fetch the current (freshly-initialized) MBQC graph for a new session
   curl -c cookies.txt http://localhost:18080/api/graph

A brand-new session starts with a small 4-node demo graph (see
:cpp:func:`get_graph_for_session` in ``server.cpp``), so you get a non-trivial
response immediately.

Apply a graph rewrite (local complementation on node 1), reusing the same
session:

.. code-block:: bash

   curl -b cookies.txt -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{"operation": "lc", "node": 1}'

Load a circuit from OpenQASM and convert it into an MBQC graph:

.. code-block:: bash

   curl -b cookies.txt -X POST http://localhost:18080/api/qasm \
     -H "Content-Type: application/json" \
     -d '{"qasm": "OPENQASM 2.0;\ninclude \"qelib1.inc\";\nqreg q[1];\nh q[0];"}'

See :doc:`../guides/examples` for a longer walkthrough covering flow-finding
and simulation, :doc:`../api/rest` for the full endpoint reference, and
:doc:`../api/cpp` for the C++ class reference.

Run the Tests
~~~~~~~~~~~~~~~~

.. code-block:: bash

   ./backend/build/test/Tests

Run it directly like this, from the repository root — not via ``ctest`` and
not from inside ``backend/build``. See :ref:`running-tests` in the build
guide for why (a relative path some tests shell out to only resolves
correctly from here) and for the one-time ``python_venv`` setup those tests need.

Next Steps
-----------

* :doc:`../guides/architecture` — how the pieces fit together
* :doc:`../guides/build` — CMake targets and build options
* :doc:`../guides/examples` — end-to-end API walkthroughs
* :doc:`../troubleshooting` — common setup problems
