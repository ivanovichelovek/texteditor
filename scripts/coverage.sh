#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-cov"
COV_DIR="$ROOT/coverage"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_CXX_COMPILER=clang++ -DCOVERAGE=ON
cmake --build "$BUILD" -j"$(nproc)" --target tests

LLVM_PROFILE_FILE="$BUILD/tests.profraw" "$BUILD/tests"

llvm-profdata merge -sparse "$BUILD/tests.profraw" -o "$BUILD/tests.profdata"

echo ""
echo "=== Coverage summary ==="
llvm-cov report "$BUILD/tests" \
    -instr-profile="$BUILD/tests.profdata" \
    -ignore-filename-regex='.*/(tests|_deps)/.*'

echo ""
echo "=== Generating HTML report ==="
llvm-cov show "$BUILD/tests" \
    -instr-profile="$BUILD/tests.profdata" \
    --format=html \
    -output-dir="$COV_DIR" \
    -ignore-filename-regex='.*/(tests|_deps)/.*'

echo "Report saved to: $COV_DIR/index.html"
