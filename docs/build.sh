#!/usr/bin/env bash
# Build the MBQuIDE backend documentation: Doxygen (C++ -> XML) then Sphinx
# (XML + .rst -> HTML). Run from anywhere; paths are resolved relative to
# this script's location.
#
# Usage:
#   bash docs/build.sh            # build HTML into docs/_build/html
#   bash docs/build.sh clean      # remove docs/doxygen and docs/_build
set -euo pipefail

DOCS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DOCS_DIR"

if [[ "${1:-}" == "clean" ]]; then
    rm -rf doxygen _build
    echo "Cleaned docs/doxygen and docs/_build."
    exit 0
fi

if ! command -v doxygen >/dev/null 2>&1; then
    echo "error: doxygen is not installed or not on PATH." >&2
    echo "  Debian/Ubuntu: sudo apt install doxygen" >&2
    echo "  macOS:         brew install doxygen" >&2
    exit 1
fi

if ! command -v sphinx-build >/dev/null 2>&1; then
    echo "error: sphinx-build is not installed or not on PATH." >&2
    echo "  Install docs dependencies first: pip install -r docs/requirements.txt" >&2
    exit 1
fi

echo "==> Running Doxygen (backend/include, backend/src -> docs/doxygen/xml)"
doxygen Doxyfile

echo "==> Running Sphinx (docs -> docs/_build/html)"
sphinx-build -b html . _build/html

echo "==> Done: docs/_build/html/index.html"
