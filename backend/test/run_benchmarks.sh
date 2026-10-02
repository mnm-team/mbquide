#!/usr/bin/env bash
# Reproduces every benchmark-derived figure in the paper's Evaluation
# section from scratch: builds the Benchmarks binary in Release mode, runs
# all three benchmark test cases (each under a virtual-memory cap - the
# dense backend is *expected* to run out of memory at the larger sizes in
# two of the three benchmarks, and without the cap that can degrade into
# swap-thrashing that stalls the machine instead of a clean, fast
# std::bad_alloc), then regenerates the PDFs directly into paper/img/.
#
# Usage (from anywhere - the script locates the repo root itself):
#   backend/test/run_benchmarks.sh
#
# Expects the python_venv/ virtual environment (see backend/README.md,
# "Run Tests") to already exist with pyzx installed, and a C++ toolchain
# able to build the project (see backend/README.md for dependencies).
#
# Total runtime: a few minutes, dominated by the ~300+ random circuits
# generated via PyZX across the three benchmarks.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

if [ ! -x "python_venv/bin/python" ]; then
    echo "error: python_venv/bin/python not found." >&2
    echo "  Set it up first: python -m venv python_venv && python_venv/bin/python -m pip install pyzx==0.10.0" >&2
    exit 1
fi

# Virtual memory cap for the benchmark process (KB). The dense backend's
# allocations fail cleanly (caught std::bad_alloc) under this cap instead
# of swapping; raise it if your machine has much less than ~4-8GB free, or
# if you widen the benchmarks' size grids in benchmark.cpp.
ULIMIT_KB=4000000

echo "==> Configuring and building Benchmarks (Release)"
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build backend/build --target Benchmarks -j"$(nproc)"

BENCH="./backend/build/test/Benchmarks"

run_case() {
    local name="$1"
    echo "==> Running: $name"
    bash -c "ulimit -v $ULIMIT_KB; \"$BENCH\" --test-case=\"$name\""
}

run_case "Benchmark: Random Clifford - Conveyor Belt Comparison"
run_case "Benchmark: Statevector vs TensorNetwork backend"
run_case "Benchmark: simplify vs greedyOptimizeEdges"

echo "==> Generating figures into paper/img/"
python_venv/bin/python backend/test/plot_full_vs_partial.py
python_venv/bin/python backend/test/plot_sv_vs_tn.py
python_venv/bin/python backend/test/plot_node_vs_edge_reduction.py

echo "==> Done. Figures written to paper/img/:"
echo "    plot_simulation_time.pdf"
echo "    plot_sv_vs_tn.pdf"
echo "    plot_node_vs_edge_reduction.pdf"
