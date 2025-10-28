#!/bin/bash

echo "StoneDB-engine Build Script"
echo "==========================="
echo ""

# create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# configure with cmake
echo "Configuring with CMake..."
cmake ..

# build
echo "Building..."
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Run './build/stone' to start the database CLI"
echo "Run './build/stone --help' to see available commands"
