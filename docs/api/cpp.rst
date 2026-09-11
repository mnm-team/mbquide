C++ API Reference
====================

Generated directly from the header comments and signatures in
``backend/include/`` via `Doxygen <https://www.doxygen.nl/>`_ and
`Breathe <https://breathe.readthedocs.io/>`_. See :doc:`../guides/architecture`
for prose context on how these pieces fit together.

.. contents::
   :local:
   :depth: 1

Graph & Flow
---------------

.. doxygenclass:: MBQC_Graph
   :members:
   :protected-members:

.. doxygenstruct:: PauliFlowResult
   :members:

.. doxygenfunction:: findPauliFlow

.. doxygenfunction:: focus

.. doxygenstruct:: GraphRewriteStep
   :members:

.. doxygenenum:: GraphRewriteRuleType

ZX-Calculus & Conversion
---------------------------

.. doxygenclass:: ZXGraph
   :members:

.. doxygenstruct:: Spider
   :members:

.. doxygenenum:: SpiderType

.. doxygenenum:: EdgeType

.. doxygenfunction:: MBQCtoZXGraph

.. doxygenfunction:: CIRCtoMBQCGraph

Quantum Circuits & QASM
---------------------------

.. doxygenclass:: QuantumCircuit
   :members:

.. doxygenstruct:: Gate
   :members:

.. doxygenclass:: QASMParser
   :members:

Simulation
-------------

.. doxygenclass:: Simulator
   :members:

.. doxygenclass:: SimulatorBackendHandle
   :members:

.. doxygenenum:: SimulatorBackendType

.. doxygenclass:: StatevectorSimulator
   :members:

.. doxygenclass:: TensorNetworkSimulator
   :members:

Output Adjustments & Utilities
----------------------------------

.. doxygenclass:: OutputAdjustmentMap
   :members:

.. doxygenenum:: MeasurementBasis

.. doxygenfile:: utils.hpp
   :sections: func
