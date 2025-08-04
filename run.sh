#!/bin/bash
set -e

echo "🔄 Cleaning old build directory..."
rm -rf build

echo "📁 Creating build directory..."
mkdir build && cd build

echo "⚙️ Running CMake..."
cmake ..

echo "🛠️ Building the project..."
make

echo "🧪 Running tests..."
ctest --verbose
