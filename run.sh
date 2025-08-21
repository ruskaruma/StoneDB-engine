#!/usr/bin/env bash
set -e  #to exit if any command fails

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

#to configure & build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

#run tests
if ctest --output-on-failure; then
    echo "All tests passed."
else
    echo "Tests failed."
fi
