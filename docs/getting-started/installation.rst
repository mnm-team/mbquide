Installation
============

There are two ways to get MBQuIDE running: download a pre-built binary, or
build it from source. This page is about the **build-from-source** path and
the environment it needs; the pre-built path needs no setup at all — see
:doc:`quickstart` for both.

Pre-built Binary
-----------------

If you just want to *run* MBQuIDE (not develop the backend itself), you don't
need anything on this page — download a release for your platform and run
it directly, no compiler or Node.js required. See :doc:`quickstart`'s
*Option 1: Run a Pre-built Binary* for the exact commands per platform.

The rest of this page covers building from source instead (:doc:`quickstart`'s
*Option 2*), which this pre-built path skips entirely.

Building From Source
---------------------

Prerequisites
~~~~~~~~~~~~~

**Backend (C++)**

* CMake ≥ 3.14
* A C++20-compatible compiler (GCC 10+, Clang 12+, or MSVC 2019+)
* `Boost Graph library <https://www.boost.org/doc/libs/latest/libs/graph/doc/>`_

All other backend dependencies — Eigen, Asio, nlohmann/json, Crow, and
doctest — are fetched automatically by CMake's ``FetchContent`` the first time
you configure the project, so you don't need to install them yourself.

**Frontend (Web UI)**

* Node.js ≥ 20
* npm ≥ 9

**Documentation (this site)** — only needed if you plan to build the docs
locally:

* Python ≥ 3.9
* `Doxygen <https://www.doxygen.nl/download.html>`_ (``apt install doxygen``,
  ``brew install doxygen``, or the Windows installer)

Clone and build
~~~~~~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/mnm-team/mbquide.git
   cd mbquide

   # 1. Build the backend
   cmake -S backend -B backend/build
   cmake --build backend/build

   # 2. Install frontend dependencies
   cd frontend && npm install && cd ..

The backend build produces a ``Server`` executable in ``backend/build/`` along
with a static library, ``libmbquide_core``, that both the server and the test
suite link against. See :doc:`../guides/build` for a breakdown of the CMake
targets and useful build flags.

Verifying the Install
-----------------------

Run the backend test suite to confirm everything compiled correctly. Run the
``Tests`` binary directly from the repository root rather than via ``ctest``
— see :ref:`running-tests` for why:

.. code-block:: bash

   ./backend/build/test/Tests

A handful of these tests also need a Python virtualenv at
``python_venv/`` (repo root) with ``pyzx`` and ``numpy`` installed; see
:ref:`running-tests` if you hit failures without one set up.

Then move on to :doc:`quickstart` to start the server.
