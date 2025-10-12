# StoneDB-engine

A modular, ACID-compliant embedded database written in C++ as a personal side project to learn databases.

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

## Building

```bash
# Quick build
./build.sh

# Or manually
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

### Interactive CLI

```bash
./build/stone
```

Commands:
- `put <key> <value>` - Store key-value pair
- `get <key>` - Retrieve value for key
- `del <key>` - Delete key
- `scan` - Show all records
- `help` - Show help
- `quit` - Exit database

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

Run the demo script to see all tests:

```bash
./demo.sh
```

Individual tests:
- `./build/test_common` - Common utilities
- `./build/test_storage` - Storage manager
- `./build/test_wal` - Write-ahead logging
- `./build/test_lockmgr` - Lock manager
- `./build/test_transaction` - Transaction manager
- `./build/test_recovery` - Crash recovery
- `./build/test_benchmark` - Performance benchmarks

## Goal

Make this into a simple embeddable DB for student or indie dev apps. Maybe release a paid binary version later.

## License

MIT License - feel free to use for learning.
