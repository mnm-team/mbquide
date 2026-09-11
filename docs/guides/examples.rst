Examples
========

End-to-end walkthroughs of the REST API, plus a note on using the core
library (``mbquide_core``) directly from C++.

Example 1: Build, Flow, and Simulate a Graph via HTTP
---------------------------------------------------------

This walks through the full lifecycle of a session: create a graph, find its
Pauli flow, and simulate it — the same sequence the web frontend performs.

.. code-block:: bash

   # Keep a cookie jar so all requests share one session
   JAR=cookies.txt

   # 1. Start a session and inspect the default demo graph
   curl -s -c $JAR http://localhost:18080/api/graph | python3 -m json.tool

.. code-block:: json

   {
     "size": 4,
     "inputs": [0],
     "outputs": [3],
     "edges": [[0, 1], [0, 2], [1, 2], [1, 3]],
     "measurements": {
       "0": {"basis": "Y", "angle": 0},
       "1": {"basis": "XY", "angle": 2.356194490192345},
       "2": {"basis": "XZ", "angle": 0.7853981633974483}
     }
   }

.. code-block:: bash

   # 2. Replace it with a custom 3-node line graph: 0 - 1 - 2
   curl -s -b $JAR -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{
           "size": 3,
           "inputs": [0],
           "outputs": [2],
           "edges": [[0, 1], [1, 2]],
           "measurements": {
             "0": {"basis": "XY", "angle": 0},
             "1": {"basis": "XY", "angle": 0}
           }
         }' | python3 -m json.tool

   # 3. Find a Pauli flow for it
   curl -s -b $JAR -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{"flow": "pauli"}' | python3 -m json.tool

The response now includes a ``"flow"`` object with correction sets
(``corrf``), odd-neighborhood correction sets (``oddNcorrf``), and
measurement depths — see :cpp:struct:`PauliFlowResult`. If ``flow.ok`` is
``false``, the graph has no valid Pauli flow and simulation will not proceed.

.. code-block:: bash

   # 4. Initialize the simulator with a fixed (non-random) all-zero input
   curl -s -b $JAR -X POST http://localhost:18080/api/sim \
     -H "Content-Type: application/json" \
     -d '{"init": true, "random": false, "input": "0"}' | python3 -m json.tool

   # 5. Run the whole simulation in one call
   curl -s -b $JAR -X POST http://localhost:18080/api/sim \
     -H "Content-Type: application/json" \
     -d '{"measureAll": true}' | python3 -m json.tool

   # ...or step through it node by node instead of measureAll:
   curl -s -b $JAR -X POST http://localhost:18080/api/sim \
     -H "Content-Type: application/json" \
     -d '{"measureNode": 0}' | python3 -m json.tool

   # 6. Read back the final simulator state at any point
   curl -s -b $JAR http://localhost:18080/api/sim | python3 -m json.tool

Example 2: Import a Circuit from OpenQASM
----------------------------------------------

Convert a small Bell-pair circuit into an MBQC graph:

.. code-block:: bash

   curl -s -b cookies.txt -X POST http://localhost:18080/api/qasm \
     -H "Content-Type: application/json" \
     -d '{
           "qasm": "OPENQASM 2.0;\ninclude \"qelib1.inc\";\nqreg q[2];\nh q[0];\ncx q[0],q[1];"
         }' | python3 -m json.tool

Internally this goes through :cpp:class:`QASMParser` → :cpp:class:`QuantumCircuit`
→ :cpp:func:`CIRCtoMBQCGraph`, replacing the session's graph. From here you can
follow the same flow/simulate steps as Example 1.

Example 3: Simplify a Graph Before Simulating
--------------------------------------------------

Larger graphs benefit from running the built-in simplification passes before
computing flow, which can reduce edge count and measurement depth:

.. code-block:: bash

   # Check first whether there's anything to optimize (read-only)
   curl -s -b cookies.txt -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{"checkOptimizeEdges": true}'
   # => {"canOptimizeEdges": true}

   curl -s -b cookies.txt -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{"simplify": true}'

   curl -s -b cookies.txt -X POST http://localhost:18080/api/graph \
     -H "Content-Type: application/json" \
     -d '{"optimizeEdges": true}'

See :doc:`../guides/architecture` for what ``simplify()`` and
``greedyOptimizeEdges()`` do under the hood (local complementation/pivot
rewrites), and :cpp:class:`MBQC_Graph` in the :doc:`../api/cpp` for the
full rewrite-rule API.

Example 4: Using ``mbquide_core`` Directly From C++
--------------------------------------------------------

If you're embedding the graph/simulation engine in another C++ program
(rather than talking to it over HTTP), link against the ``mbquide_core``
CMake target and skip Crow entirely:

.. code-block:: cmake

   add_subdirectory(path/to/mbquide/backend)  # or add_subdirectory to just mbquide_core
   target_link_libraries(my_app PRIVATE mbquide_core)

.. code-block:: cpp

   #include "MBQC_Graph.hpp"
   #include "Flow.hpp"
   #include "Simulator.hpp"

   int main() {
       MBQC_Graph graph(3, /*inputs=*/{0}, /*outputs=*/{2});
       graph.addEdge(0, 1);
       graph.addEdge(1, 2);
       graph.setMeasurement(0, MeasurementBasis::XY, 0.0);
       graph.setMeasurement(1, MeasurementBasis::XY, 0.0);

       PauliFlowResult flow = findPauliFlow(graph);
       if (!flow.ok) {
           return 1;
       }

       Simulator sim(graph, flow, /*random=*/false, /*input=*/"0");
       sim.simulateAll();

       std::cout << sim.toJson().dump(2) << std::endl;
   }

This is the same code path exercised by ``backend/test/*.cpp`` — those files
are a good source of further usage patterns for each class.
