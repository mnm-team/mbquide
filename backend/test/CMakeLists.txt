# -----------------------------
# Tests
# -----------------------------
add_executable(Tests
    test_main.cpp
    test_MBQC.cpp
    test_outAdj.cpp
    test_Statevector.cpp
    test_Tensornetwork.cpp
    test_Simulator.cpp
    test_Flow.cpp
    test_simplifications.cpp
)

target_link_libraries(Tests
PRIVATE
    mbquide_core
    doctest::doctest
)

add_test(NAME Tests COMMAND Tests)


# -----------------------------
# Benchmarks
# -----------------------------
add_executable(Benchmarks
    test_main.cpp
    benchmark.cpp
)

target_link_libraries(Benchmarks
PRIVATE
    mbquide_core
    doctest::doctest
)


# -----------------------------
# Scenarios (Statevector vs TensorNetwork backend, with block visualization)
# -----------------------------
add_executable(TensornetInfo
    test_main.cpp
    tensornet_info.cpp
)

target_link_libraries(TensornetInfo
PRIVATE
    mbquide_core
    doctest::doctest
)