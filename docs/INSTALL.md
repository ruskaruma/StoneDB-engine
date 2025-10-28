# Installation Guide

## System Requirements

- **OS**: Linux, macOS, or Unix-like system
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Build System**: CMake 3.16+
- **Make**: Standard build tools

## Step-by-Step Installation

### 1. Install Prerequisites

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Or use Homebrew
brew install cmake
```

#### Fedora/RHEL/CentOS
```bash
sudo dnf install gcc-c++ cmake make
```

#### Arch Linux
```bash
sudo pacman -S base-devel cmake
```

### 2. Clone the Repository

```bash
git clone https://github.com/ruskaruma/StoneDB-engine.git
cd StoneDB-engine
```

### 3. Build the Project

#### Option A: Using the build script (recommended)
```bash
chmod +x build.sh
./build.sh
```

#### Option B: Manual build
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)  # Use -j4 or appropriate number for your CPU
```

### 4. Verify Installation

```bash
./build/stone --help
```

You should see the help message. If you see an error, check:
- CMake is installed: `cmake --version`
- C++ compiler is available: `g++ --version` or `clang++ --version`
- Build completed without errors

### 5. Run Tests (Optional)

```bash
cd build
./test_common
./test_storage
# ... run other tests
```

## Troubleshooting

### CMake not found
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake
```

### Compiler errors
- Ensure you have a C++17 compatible compiler
- Check version: `g++ --version` (should be 7.0+)

### Permission denied on build.sh
```bash
chmod +x build.sh
```

### Build fails with "No rule to make target"
Clean and rebuild:
```bash
rm -rf build
mkdir build && cd build
cmake ..
make
```

## Next Steps

After successful installation:

1. Read [README.md](README.md) for usage examples
2. Check [USAGE_GUIDE.md](USAGE_GUIDE.md) for detailed documentation
3. See [CONTRIBUTING.md](CONTRIBUTING.md) if you want to contribute

## Uninstallation

Simply delete the repository:
```bash
rm -rf StoneDB-engine
```

No system files are modified during installation.
