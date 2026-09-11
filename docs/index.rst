MBQuIDE Documentation
=======================

**MBQuIDE** (Measurement-Based Quantum Interactive Development Environment) is an
interactive graphical editor for measurement-based quantum computing (MBQC). It
consists of a **C++ REST API backend** — which exposes the MBQC graph,
ZX-calculus, and simulation engine over HTTP — and a **React/TypeScript web
frontend** that renders and interacts with it.

This site is primarily a **backend developer reference** (architecture, build
system, REST/C++ API, examples), with one dedicated page covering the
:doc:`frontend <frontend>`: its tech stack, project layout, and how to use the
editor itself.

The backend is built on `Crow <https://crowcpp.org/>`_ for HTTP routing,
`nlohmann/json <https://github.com/nlohmann/json>`_ for request/response
serialization, `Eigen <https://eigen.tuxfamily.org/>`_ for linear algebra, and the
`Boost Graph Library <https://www.boost.org/doc/libs/latest/libs/graph/doc/html/graph/index.html>`_ for
graph algorithms. It maintains one MBQC graph, ZX-graph, flow, and simulator per
browser session, and serves the compiled frontend as static files. The frontend
is a Vite + React 19 + TypeScript single-page app that talks to the backend over
that REST API.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   getting-started/installation
   getting-started/quickstart

.. toctree::
   :maxdepth: 2
   :caption: Guides

   guides/architecture
   guides/build
   guides/examples

.. toctree::
   :maxdepth: 2
   :caption: Frontend

   frontend

.. toctree::
   :maxdepth: 3
   :caption: Reference

   api/rest
   api/cpp
   troubleshooting

Indices and Tables
-------------------

* :ref:`genindex`
* :ref:`search`
