Building the Backend
=======================

This guide covers the CMake targets, useful build options, and how to build
this documentation site itself.

CMake Targets
---------------

``backend/CMakeLists.txt`` defines:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Target
     - Description
   * - ``mbquide_core``
     - Static library containing everything under ``backend/src`` except
       ``server.cpp`` — the graph, flow, ZX-calculus, QASM parser, and
       simulator code. Framework-agnostic (no Crow dependency).
   * - ``Server``
     - The REST API executable (``backend/src/server.cpp``), linked against
       ``mbquide_core`` and ``Crow::Crow``.
   * - ``Tests``
     - doctest-based unit tests (``backend/test/*.cpp``), defined in
       ``backend/test/CMakeLists.txt``.
   * - ``Benchmarks``
     - Performance benchmarks for the simplification/simulation code
       (``backend/test/benchmark.cpp``).
   * - ``TensornetInfo``
     - A scenario runner comparing the statevector and tensor-network
       simulator backends (``backend/test/tensornet_info.cpp``).

.. note::
   ``Server`` builds straight to ``backend/build/Server``, but ``Tests``,
   ``Benchmarks``, and ``TensornetInfo`` build to ``backend/build/test/`` —
   CMake mirrors the source layout (they're defined in
   ``backend/test/CMakeLists.txt``) into the build tree. Get the path wrong
   and the shell just reports "no such file".

Basic Build
-------------

.. code-block:: bash

   cmake -S backend -B backend/build
   cmake --build backend/build

This configures and builds everything, including ``Tests`` and
``Benchmarks``, since ``enable_testing()`` + ``add_subdirectory(test)`` run
unconditionally.

To build a single target instead of everything:

.. code-block:: bash

   cmake --build backend/build --target Server
   cmake --build backend/build --target Tests

Useful CMake Options
-----------------------

.. code-block:: bash

   # Faster, parallel build
   cmake --build backend/build -j"$(nproc)"

   # Debug build (default build type is unset/empty, i.e. no optimization flags)
   cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Debug

   # Release build
   cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release

Dependencies are declared with ``FetchContent`` and downloaded/built on first
configure:

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Dependency
     - Pinned version
     - Purpose
   * - Eigen
     - 5.0.1
     - Dense linear algebra for the statevector simulator.
   * - Asio
     - asio-1-38-0
     - Standalone Asio, required by Crow.
   * - nlohmann/json
     - v3.12.0
     - Request/response (de)serialization.
   * - Crow
     - v1.3.2
     - HTTP routing for ``Server``.
   * - doctest
     - v2.5.2
     - Unit-testing framework for ``Tests``/``Benchmarks``.

``Boost::graph`` is the one dependency **not** vendored — it must be
installed on your system (``find_package(Boost REQUIRED COMPONENTS graph)``).
See :doc:`../troubleshooting` if CMake can't find it.

Because everything else is fetched from source, the first configure requires
network access and takes noticeably longer than subsequent ones (CMake caches
the fetched sources under ``backend/build/_deps``).

.. _running-tests:

Running Tests
---------------

The ``Tests`` target builds to ``backend/build/test/Tests`` (it lives under
``test/`` because that's where ``backend/test/CMakeLists.txt`` — pulled in via
``add_subdirectory(test)`` — defines it; CMake mirrors that into the build
tree). Run it **directly from the repository root**, not via ``ctest`` and
not from inside ``backend/build/test/``:

.. code-block:: bash

   ./backend/build/test/Tests

   # doctest's own CLI works too, e.g. to filter by test case:
   ./backend/build/test/Tests --test-case="*Flow*"

.. warning::
   ``ctest`` does **not** work reliably for this project — running it from
   ``backend/build`` (the normal way) reports dozens of spurious failures, all
   in the tensor-comparison tests (``greedyOptimizeEdges() preserves the
   tensor...`` and similar). This isn't a real bug: those tests call
   ``compareTensors()``/``randomClifford()`` (``backend/include/
   test_helpers.hpp``), which shell out to a **relative** path,
   ``python_venv/bin/python``. ``ctest`` runs tests with their working
   directory set to wherever the test binary was built
   (``backend/build/test/``), where that relative path doesn't resolve, so
   every Python-backed check silently fails. Running the binary directly from
   the repository root (as above) doesn't have this problem, since
   ``python_venv/bin/python`` then resolves correctly. There's no ``ctest``
   flag to fix this from the command line (the working directory is set
   per-test at ``add_test()`` time, which this project doesn't override), so
   just use the binary directly. See :doc:`../troubleshooting` for the same
   explanation if you hit this without reading here first.

These Python-backed checks also require ``python_venv/`` to exist at the
repository root with ``pyzx`` and ``numpy`` installed — see `Setting Up
python_venv`_ below. Without it, every test that calls ``compareTensors()``/
``randomClifford()`` fails the same way (the shelled-out command simply can't
run), even when invoked correctly from the repository root.

Setting Up ``python_venv``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A handful of tests (``backend/test/test_simplifications.cpp`` and others that
verify a graph rewrite preserves the underlying tensor) don't check this in
C++ — they export both graphs to PyZX's JSON format and shell out to a Python
script (``backend/test/compare_tensors.py``, plus
``backend/test/random_clifford.py`` for randomized test inputs) that uses
`pyzx <https://pyzx.readthedocs.io/>`_ to compare/generate them. That script
is invoked via the **hardcoded relative path** ``python_venv/bin/python``, so
the virtualenv must live at the repository root, named exactly
``python_venv``:

.. code-block:: bash

   python3 -m venv python_venv
   python_venv/bin/pip install pyzx numpy

This is a one-time setup step, separate from (and unrelated to) the
documentation virtualenv described in :ref:`building-the-documentation`.

Running Benchmarks
--------------------

Like ``Tests``, ``Benchmarks`` builds to ``backend/build/test/`` rather than
``backend/build/`` directly:

.. code-block:: bash

   ./backend/build/test/Benchmarks

.. _building-the-documentation:

Building the Documentation
-----------------------------

This site (the one you're reading) lives in ``docs/`` and is built with
Sphinx + Breathe, using Doxygen to extract the C++ API surface.

Prerequisites: Python ≥ 3.9, and `Doxygen <https://www.doxygen.nl/>`_
installed and on ``PATH``.

.. code-block:: bash

   cd docs
   python3 -m venv .venv && source .venv/bin/activate
   pip install -r requirements.txt

   # Build (runs Doxygen, then Sphinx)
   make html
   # or, equivalently:
   bash build.sh

   # Open the result
   xdg-open _build/html/index.html   # Linux
   open _build/html/index.html       # macOS

``make html`` (and ``build.sh``) always regenerate the Doxygen XML first, so
the API reference stays in sync with the headers. See
:doc:`../troubleshooting` if Doxygen or Breathe fail to pick up a class.

Read the Docs builds this same site automatically from
``.readthedocs.yaml`` at the repository root on every push — see that file
for the exact ``pre_build`` step that invokes Doxygen there.
