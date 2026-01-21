# StoneDB-engine

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C++-17-orange.svg)

This is an ACID-compliant embedded database written in C++ as a personal side project to learn databases at their core.

## Features

- **ACID Compliance**: Full transaction support with atomicity, consistency, isolation, and durability
- **Page-based Storage**: Efficient disk-based storage with page caching
- **Write-Ahead Logging (WAL)**: Crash recovery and durability guarantees
- **Lock Manager**: Concurrency control with deadlock detection
- **Transaction Manager**: Complete transaction lifecycle management
- **Interactive CLI**: Simple command-line interface for database operations

## Quick Build

```bash
git clone https://github.com/ruskaruma/StoneDB-engine.git
cd StoneDB-engine
./build.sh
./build/stone
```

## Documentation

See [docs/](docs/) on how to get started:

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System architecture
- [docs/API.md](docs/API.md) - API documentation
- [docs/TUTORIAL.md](docs/TUTORIAL.md) - How I built this

## License

MIT License
