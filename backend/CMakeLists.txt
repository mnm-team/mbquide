cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(MBQuIDE-Backend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -----------------------------
# Eigen
# -----------------------------
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 5.0.1
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

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

# -----------------------------
# nlohmann/json
# -----------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# -----------------------------
# Crow
# -----------------------------
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.3.2
)

FetchContent_MakeAvailable(crow)

# -----------------------------
# doctest
# -----------------------------
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.2
)
FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
    FetchContent_Populate(doctest)
endif()

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${doctest_SOURCE_DIR}/doctest)

add_library(doctest::doctest ALIAS doctest)

# -----------------------------
# Boost
# -----------------------------
find_package(Boost REQUIRED COMPONENTS graph)

# -----------------------------
# Core library (shared by server + tests)
# -----------------------------
add_library(mbquide_core)

file(GLOB serverHeaders "include/*.h")

target_sources(mbquide_core
 PRIVATE
   src/utils.cpp
   src/graph/Flow.cpp
   src/graph/MBQC_Graph.cpp
   src/graph/OutputAdjustments.cpp
   src/QASM/MBQC2ZX.cpp
   src/QASM/QASM_Parser.cpp
   src/QASM/ZX2MBQC.cpp
   src/QASM/ZX_Graph.cpp
 PUBLIC
   FILE_SET HEADERS
   BASE_DIRS include
   FILES ${serverHeaders}
)

target_link_libraries(mbquide_core
PUBLIC
    Eigen3::Eigen
    Boost::graph
    asio
    nlohmann_json::nlohmann_json
    # Only link ws2_32 on Windows
    $<$<OS:Windows>:ws2_32>
)

target_include_directories(mbquide_core PUBLIC include)

# -----------------------------
# Server executable
# -----------------------------
add_executable(Server src/server.cpp)

target_link_libraries(Server
PRIVATE
    mbquide_core
    Crow::Crow
    # Only link ws2_32 on Windows
    $<$<OS:Windows>:ws2_32>
)

# -----------------------------
# Tests
# -----------------------------
enable_testing()
add_subdirectory(test)