cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(MBQuIDE-Backend)

set(CMAKE_CXX_STANDARD 17)

# create the executable
add_executable(Server)

file(GLOB serverHeaders
     "include/*.h"
)

target_sources(Server 
 PRIVATE 
   src/server.cpp
   src/utils.cpp
   src/graph/Flow.cpp
   src/graph/MBQC_Graph.cpp
   src/graph/OutputAdjustments.cpp
   src/QASM/MBQC2ZX.cpp
   src/QASM/QASM_Parser.cpp
   src/QASM/Quantum_Circuit.cpp
   src/QASM/ZX2MBQC.cpp
   src/QASM/ZX_Graph.cpp
 PUBLIC
   FILE_SET HEADERS
   BASE_DIRS
     include
   FILES
     ${serverHeaders}
)
