# Contributing to StoneDB-engine

Thanks for your interest in contributing to StoneDB-engine! This project is a great place to learn about database internals and open source contributions.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/StoneDB-engine.git`
3. Create a branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Build and test: `./build.sh && cd build && ./test_common && ./test_storage`
6. Commit: `git commit -m "feat: your feature description"`
7. Push: `git push origin feature/your-feature-name`
8. Open a Pull Request

## Code Style

- Use 2-space indentation
- camelCase for variables and functions
- PascalCase for classes
- Keep lines under 100 characters
- Add comments sparingly but meaningfully
- Use smart pointers for memory management

## Building

```bash
./build.sh
```

## Running Tests

```bash
cd build
./test_common
./test_storage
./test_wal
./test_lockmgr
./test_transaction
```

## Areas for Contribution

See the issues page for beginner-friendly, intermediate, and advanced tasks!
