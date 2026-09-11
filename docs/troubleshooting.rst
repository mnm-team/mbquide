Troubleshooting
================

Build Issues
-------------

**CMake can't find Boost / ``find_package(Boost ... graph)`` fails**

.. code-block:: text

   Could NOT find Boost (missing: graph)

Boost is the one backend dependency that isn't fetched automatically — install
it via your system package manager:

.. code-block:: bash

   # Debian/Ubuntu
   sudo apt install libboost-graph-dev

   # macOS (Homebrew)
   brew install boost

   # Windows (vcpkg)
   vcpkg install boost-graph

If CMake still can't find it after installing, point it at the install
prefix explicitly:

.. code-block:: bash

   cmake -S backend -B backend/build -DBOOST_ROOT=/path/to/boost

**``FetchContent`` fails to download a dependency**

The first configure needs network access to pull Eigen, Asio, nlohmann/json,
Crow, and doctest. Behind a proxy or on a restricted network, set the usual
Git/HTTP proxy environment variables before running ``cmake``, or pre-populate
``backend/build/_deps`` from a machine that has access.

**Stale CMake cache after changing dependency versions or moving the repo**

.. code-block:: bash

   rm -rf backend/build
   cmake -S backend -B backend/build
   cmake --build backend/build

**Compiler errors about missing C++20 features**

Confirm your compiler is new enough (GCC 10+, Clang 12+, MSVC 2019+) and that
CMake picked it up:

.. code-block:: bash

   cmake -S backend -B backend/build -DCMAKE_CXX_COMPILER=g++-12

**``ctest`` reports dozens of failures, all tensor-comparison tests**

.. code-block:: text

   TEST CASE:  greedyOptimizeEdges() preserves the tensor on hand-built graphs
   ...
   ERROR: CHECK( compareTensors(MBQCtoZXGraph(graph), MBQCtoZXGraph(original)) ) is NOT correct!

This is a working-directory mismatch, not a real failure. Those tests call
``compareTensors()``/``randomClifford()`` (``backend/include/
test_helpers.hpp``), which shell out to the **relative** path
``python_venv/bin/python``. ``ctest`` runs the ``Tests`` binary with its
working directory set to wherever CMake placed it — ``backend/build/test/``
— so that relative path doesn't resolve there, and every Python-backed check
fails. There's no ``ctest`` flag to change a test's working directory from
the command line (it's set per-test in ``add_test()``/
``set_tests_properties()``, which this project doesn't override), so the fix
is simply to skip ``ctest`` and run the binary directly from the repository
root instead, where the relative path does resolve:

.. code-block:: bash

   ./backend/build/test/Tests

See :ref:`running-tests` in the build guide for more.

**Tensor-comparison tests fail even when run from the repository root**

If ``./backend/build/test/Tests`` still fails on ``compareTensors``/
``randomClifford``-based tests, ``python_venv/`` likely doesn't exist yet, or
is missing ``pyzx``/``numpy``:

.. code-block:: bash

   python3 -m venv python_venv
   python_venv/bin/pip install pyzx numpy

See :ref:`running-tests` for details — this is a separate virtualenv from the
one used to build this documentation site.

Runtime Issues
----------------

**Frontend can't reach the API / CORS errors in the browser console**

The server only sends CORS headers for ``http://localhost:5173`` (see the
``CORS`` middleware in ``server.cpp``). If you're serving the frontend from a
different origin, either run it on port 5173, use the built-in static-file
serving (``./backend/build/Server`` after ``npm run build``), or update the
allowed origin in ``server.cpp`` and rebuild.

**API requests fail with 500 and no session state persists between requests**

Session state is keyed by the ``session_id`` cookie. If your client doesn't
send/store cookies (a raw ``fetch``/``curl`` without a cookie jar, or a
browser blocking third-party cookies across origins), every request looks
like a brand-new session. Use ``curl -c cookies.txt -b cookies.txt`` (see
:doc:`guides/examples`), or ensure your frontend's HTTP client has
``credentials: "include"`` set.

**``/api/graph`` or ``/api/sim`` returns an empty/default response**

Several endpoints depend on state from an earlier call in the same session:

* Simulating requires a Pauli flow to already exist (``{"flow": "pauli"}``
  first) — see ``flow.ok`` in the response.
* ``GET /api/sim`` returns a default simulator until ``POST /api/sim`` with
  ``{"init": true, ...}`` has been called at least once.

**Server exits immediately / port 18080 already in use**

Another process (perhaps a previous ``Server`` instance) is already bound to
the port:

.. code-block:: bash

   # Linux/macOS: find and stop the process holding the port
   lsof -i :18080
   kill <pid>

**Frontend loads but shows a blank page / 404 for static assets**

``Server`` serves the frontend from ``./frontend/dist`` *relative to the
working directory it was launched from*. Run it from the repository root, or
build the frontend first (``cd frontend && npm run build``).

Frontend Issues
------------------

**Frontend loads but every API call fails (even with matching CORS)**

The frontend calls the backend at a hardcoded ``http://localhost:18080``
(see :doc:`frontend`'s *Connecting to the Backend* section) — there is no
``.env``/build-time variable for this. If you run the backend on a different
port, or reverse-proxy it under a different host, frontend API calls will
silently fail (or hit the wrong service) until the hardcoded URLs in
``src/apps/MBQC/api/graphApi.ts`` and the other ``fetch(...)`` call sites are
updated and the frontend is rebuilt.

**``npm install`` fails or the dev server won't start**

Confirm Node.js ≥ 20 and npm ≥ 9 (``node -v``, ``npm -v``); Vite 6 and
React 19 both require a reasonably current Node. Delete
``frontend/node_modules`` and ``frontend/package-lock.json`` conflicts by
re-running ``npm install`` from a clean checkout if a previous partial
install left things inconsistent.

Documentation Build Issues
-----------------------------

**``doxygenclass``/``doxygenfunction`` directives render "Cannot find class"**

Breathe couldn't find that symbol in the Doxygen XML. Usually this means:

* Doxygen hasn't been run yet, or its output is stale — delete
  ``docs/doxygen`` and rebuild (``make html`` / ``bash build.sh`` re-run
  Doxygen automatically; a bare ``sphinx-build`` does not).
* The symbol name doesn't match exactly (case-sensitive, and template/
  namespace-qualified names must match Doxygen's mangling).

.. code-block:: bash

   cd docs
   rm -rf doxygen
   doxygen Doxyfile
   grep -r "MyClassName" doxygen/xml/*.xml   # confirm Doxygen actually saw it

**``ModuleNotFoundError: No module named 'breathe'`` (or ``sphinx_rtd_theme``)**

The docs virtualenv isn't active, or dependencies weren't installed:

.. code-block:: bash

   cd docs
   python3 -m venv .venv && source .venv/bin/activate
   pip install -r requirements.txt

**``doxygen: command not found``**

Doxygen is a system tool, not a Python package, so it isn't in
``requirements.txt``:

.. code-block:: bash

   # Debian/Ubuntu
   sudo apt install doxygen

   # macOS
   brew install doxygen

On Read the Docs this is handled by ``apt_packages`` in
``.readthedocs.yaml`` — no local action needed for RTD builds themselves.

**Read the Docs build fails but a local build succeeds**

Check the RTD build log for the ``pre_build`` job output specifically — if
Doxygen fails there (e.g. a header with invalid syntax under
``EXTRACT_ALL``), the XML will be missing or incomplete and every
``doxygen*`` directive downstream will fail. Reproduce locally with:

.. code-block:: bash

   cd docs && rm -rf doxygen && doxygen Doxyfile && sphinx-build -b html . _build/html

Getting More Help
--------------------

If none of the above resolves your issue, please open an issue on the
`GitHub repository <https://github.com/mnm-team/mbquide/issues>`_ with:

* Your OS, compiler/CMake version (or Python/Sphinx version for docs issues)
* The exact command you ran and its full output
* Whether the issue reproduces with a clean ``backend/build`` (or
  ``docs/doxygen`` + ``docs/_build``) directory
