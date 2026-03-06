cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(MBQuIDE-Backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -----------------------------
# Eigen
# -----------------------------
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 5.0.0
)

FetchContent_MakeAvailable(eigen)

# -----------------------------
# Asio
# -----------------------------
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-38-0
)

FetchContent_MakeAvailable(asio)

# Create interface target for standalone Asio
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

# -----------------------------
# nlohmann/json
# -----------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

# -----------------------------
# Boost
# -----------------------------
find_package(Boost REQUIRED COMPONENTS graph)

# -----------------------------
# Server executable
# -----------------------------
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
   BASE_DIRS include
   FILES ${serverHeaders}
)

# -----------------------------
# Link dependencies
# -----------------------------
target_link_libraries(Server
PRIVATE
    Eigen3::Eigen
    Boost::graph
    asio
    nlohmann_json::nlohmann_json
)