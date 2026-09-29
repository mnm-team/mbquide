Graph & Flow
===============

The MBQC graph state itself, and Pauli-flow finding/focusing on it. See
:doc:`../../guides/architecture` for how these fit into the rest of the
backend.

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
