ZX-Calculus & Conversion
===========================

The ZX-calculus diagram type and its conversions to/from
:cpp:class:`MBQC_Graph`. Not part of the circuit-to-pattern pipeline (see
:doc:`circuits`) — this exists for the frontend's ``/ZX`` diagram view and as
a tensor-comparison oracle in tests. See :doc:`../../guides/architecture` for
details.

.. doxygenclass:: ZXGraph
   :members:

.. doxygenstruct:: Spider
   :members:

.. doxygenenum:: SpiderType

.. doxygenenum:: EdgeType

.. doxygenfunction:: MBQCtoZXGraph
