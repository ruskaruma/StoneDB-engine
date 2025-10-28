# StoneDB-engine

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C++-17-orange.svg)
![Version](https://img.shields.io/badge/version-1.0.0-green.svg)

A modular, ACID-compliant embedded database written in C++ as a personal side project to learn databases.

## Prerequisites

- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.16 or higher
- Make (or equivalent build system)
- Linux, macOS, or Unix-like system

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

**macOS:**
```bash
brew install cmake
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ cmake make
```

## Features

- **ACID Compliance**: Full transaction support with atomicity, consistency, isolation, and durability
- **Page-based Storage**: Efficient disk-based storage with page caching
- **Write-Ahead Logging (WAL)**: Crash recovery and durability guarantees
- **Lock Manager**: Concurrency control with deadlock detection
- **Transaction Manager**: Complete transaction lifecycle management
- **Interactive CLI**: Simple command-line interface for database operations

## Architecture

StoneDB-engine is built with a modular architecture:

- **StorageManager**: Handles page-based storage and caching
- **WALManager**: Write-ahead logging for crash recovery
- **LockManager**: Concurrency control and deadlock detection
- **TransactionManager**: ACID transaction management

## Installation

### Clone the Repository

```bash
git clone https://github.com/ruskaruma/StoneDB-engine.git
cd StoneDB-engine
```

### Build

```bash
# Quick build (recommended)
chmod +x build.sh
./build.sh

# Or manually
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Verify Installation

After building, the executable will be at `./build/stone`. Test it:

```bash
./build/stone --help
```

You should see the help message. The build is complete!

## Quick Start

After building, you can immediately start using the database:

```bash
# Start with default database (stonedb.sdb)
./build/stone

# Or specify a custom database file
./build/stone --db myapp.sdb

# Batch mode for scripts
echo "put key value" | ./build/stone --db myapp.sdb --batch
```

You'll see the interactive prompt:
```
StoneDB-engine v1.0.0
Database: myapp.sdb
Type 'help' for commands
stonedb> 
```

## Usage

### Interactive CLI

```bash
./build/stone --db myapp.sdb
```

**Commands:**
- `put <key> <value>` - Store key-value pair
- `get <key>` - Retrieve value for key
- `del <key>` - Delete key
- `scan` - Show all records
- `backup <path>` - Backup database to JSON
- `restore <path>` - Restore database from JSON
- `help` - Show help
- `quit` - Exit database

### Command-Line Options

- `-d, --db PATH` - Database file path (default: stonedb.sdb)
- `-b, --batch` - Batch mode (non-interactive)
- `-q, --quiet` - Suppress log messages
- `-h, --help` - Show help message

### Comprehensive Usage Guide

For detailed usage examples, scripting, integration, and real-world patterns, see **[USAGE_GUIDE.md](USAGE_GUIDE.md)**.

### Example Session

#### Basic Operations
```
stonedb> put hello world
OK
stonedb> put test 123
OK
stonedb> get hello
world
stonedb> scan
hello = world
test = 123
stonedb> quit
Goodbye!
```

#### Updating Records
```
stonedb> put user alice
OK
stonedb> get user
alice
stonedb> put user bob
OK
stonedb> get user
bob
```

#### Deleting Records
```
stonedb> put temp data
OK
stonedb> get temp
data
stonedb> del temp
OK
stonedb> get temp
NOT FOUND
```

#### Scanning All Records
```
stonedb> put key1 value1
OK
stonedb> put key2 value2
OK
stonedb> put key3 value3
OK
stonedb> scan
key1 = value1
key2 = value2
key3 = value3
```

## Testing

After building, run the test suite:

```bash
cd build
./test_common
./test_storage
./test_wal
./test_lockmgr
./test_transaction
./test_recovery
./test_benchmark
```

Individual tests:
- `test_common` - Common utilities
- `test_storage` - Storage manager
- `test_wal` - Write-ahead logging
- `test_lockmgr` - Lock manager
- `test_transaction` - Transaction manager
- `test_recovery` - Crash recovery
- `test_benchmark` - Performance benchmarks

## Goal

Make this into a simple embeddable DB for student or indie dev apps. Maybe release a paid binary version later.

## Documentation

- [INSTALL.md](INSTALL.md) - Detailed installation instructions
- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture with diagrams
- [USAGE_GUIDE.md](USAGE_GUIDE.md) - Comprehensive usage guide
- [docs/API.md](docs/API.md) - API documentation
- [docs/TUTORIAL.md](docs/TUTORIAL.md) - Architecture tutorial

## License

MIT License - feel free to use for learning.
