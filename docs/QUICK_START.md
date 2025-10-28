# Quick Start Guide

## 1. Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 2. Basic Usage

### Interactive Mode
```bash
./build/stone --db myapp.sdb
stonedb> put name John
stonedb> get name
stonedb> scan
stonedb> quit
```

### Batch Mode (Scripting)
```bash
# Single command
echo "put status online" | ./build/stone --db myapp.sdb --batch

# Multiple commands
cat > setup.txt << EOF
put app:name MyApp
put app:version 1.0
scan
