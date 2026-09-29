Quantum Circuits & QASM
===========================

Gate-list circuits, their OpenQASM 2.0 parser, and the direct translation
into an MBQC pattern (``Circ2MBQC.hpp``, Broadbent & Kashefi's method — see
:doc:`../../guides/architecture`).

.. doxygenclass:: QuantumCircuit
   :members:

.. doxygenstruct:: Gate
   :members:

.. doxygenclass:: QASMParser
   :members:

.. doxygenfunction:: CIRCtoMBQCGraph
