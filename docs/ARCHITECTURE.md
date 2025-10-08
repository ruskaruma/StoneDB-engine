# StoneDB-engine Architecture

## Overview

StoneDB-engine is a modular embedded database designed to understand real database internals. It implements core database concepts including ACID compliance, crash recovery, and concurrency control.

## System Architecture

```mermaid
graph TB
    CLI[CLI Interface<br/>main.cpp]
    TXN[Transaction Manager<br/>transaction.cpp]
    LOCK[Lock Manager<br/>lockmgr.cpp]
    WAL[WAL Manager<br/>wal.cpp]
    STORAGE[Storage Manager<br/>storage.cpp]
    DISK[(Disk Storage<br/>.sdb & .wal files)]
    
    CLI -->|begin/commit/abort| TXN
    CLI -->|put/get/delete| TXN
    
    TXN -->|acquire/release locks| LOCK
    TXN -->|log operations| WAL
    TXN -->|read/write data| STORAGE
    
    WAL -->|persist log| DISK
    STORAGE -->|read/write pages| DISK
    
    style CLI fill:#e1f5ff
    style TXN fill:#fff4e1
    style LOCK fill:#ffe1e1
    style WAL fill:#e1ffe1
    style STORAGE fill:#f0e1ff
    style DISK fill:#f5f5f5
```

## Transaction Flow

```mermaid
sequenceDiagram
    participant User
    participant CLI
    participant TxnMgr as Transaction Manager
    participant LockMgr as Lock Manager
    participant WAL as WAL Manager
    participant Storage as Storage Manager
    
    User->>CLI: put("user1", "alice")
    CLI->>TxnMgr: beginTransaction()
    TxnMgr->>WAL: log BEGIN_TXN
    WAL-->>TxnMgr: txnId=1
    TxnMgr-->>CLI: txnId=1
    
    CLI->>TxnMgr: putRecord(1, "user1", "alice")
    TxnMgr->>LockMgr: acquireLock(1, "user1", EXCLUSIVE)
    LockMgr-->>TxnMgr: lock granted
    TxnMgr->>WAL: log PUT_RECORD
    TxnMgr->>Storage: putRecord("user1", "alice")
    Storage-->>TxnMgr: success
    TxnMgr-->>CLI: success
    
    CLI->>TxnMgr: commitTransaction(1)
    TxnMgr->>WAL: log COMMIT_TXN
    TxnMgr->>Storage: flushAll()
    Storage-->>TxnMgr: flushed
    TxnMgr->>LockMgr: releaseAllLocks(1)
    TxnMgr-->>CLI: committed
    CLI-->>User: OK
```

## ACID Implementation

```mermaid
graph LR
    A[Atomicity] -->|WAL ensures<br/>all-or-nothing| WAL[WAL Manager]
    C[Consistency] -->|Prevents<br/>conflicts| LOCK[Lock Manager]
    I[Isolation] -->|Two-phase<br/>locking| LOCK
    D[Durability] -->|Flush to<br/>disk| STORAGE[Storage Manager]
    
    style A fill:#ffe1e1
    style C fill:#e1ffe1
    style I fill:#e1f5ff
    style D fill:#fff4e1
```

## Core Components

### StorageManager
- **Purpose**: Handles persistent storage using page-based architecture
- **Features**: 
  - Page caching for performance
  - Record-level operations (put/get/delete)
  - Simple allocation strategy
- **Files**: `src/storage.cpp`, `include/storage.hpp`

### WALManager (Write-Ahead Logging)
- **Purpose**: Ensures durability and crash recovery
- **Features**:
  - Logs all operations before applying to storage
  - Replay capability for recovery
  - Transaction logging (begin/commit/abort)
- **Files**: `src/wal.cpp`, `include/wal.hpp`

### LockManager
- **Purpose**: Concurrency control and deadlock prevention
- **Features**:
  - Shared and exclusive locks
  - Deadlock detection using cycle detection
  - Lock upgrading (shared to exclusive)
- **Files**: `src/lockmgr.cpp`, `include/lockmgr.hpp`

### TransactionManager
- **Purpose**: ACID transaction management
- **Features**:
  - Transaction lifecycle (begin/commit/abort)
  - Read and write sets tracking
  - Integration with storage, WAL, and lock manager
- **Files**: `src/transaction.cpp`, `include/transaction.hpp`

## Data Flow

1. **Transaction Begin**: Logged in WAL
2. **Operations**: 
   - Acquire appropriate locks
   - Log operations in WAL
   - Apply to storage
3. **Commit**: 
   - Log commit in WAL
   - Release locks
   - Mark transaction complete
4. **Abort**: 
   - Log abort in WAL
   - Release locks
   - Discard changes

## Concurrency Control

- **Locking**: Two-phase locking with deadlock detection
- **Isolation**: Serializable isolation level
- **Deadlock Detection**: Cycle detection algorithm
- **Lock Types**: Shared (read) and Exclusive (write)

## Crash Recovery

- **WAL Replay**: Replay committed transactions on startup
- **Atomicity**: Uncommitted transactions are discarded
- **Durability**: Committed transactions survive crashes

## Performance Considerations

- **Page Caching**: In-memory page cache for frequently accessed data
- **Lock Granularity**: Record-level locking for fine-grained concurrency
- **WAL Batching**: Multiple operations per WAL entry
- **Memory Management**: RAII and smart pointers for safety
