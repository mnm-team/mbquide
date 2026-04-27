# -----------------------------
# Tests
# -----------------------------
add_executable(Tests
    test_main.cpp
    test_MBQC.cpp
    test_outAdj.cpp
    test_Statevector.cpp
    test_Simulator.cpp
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