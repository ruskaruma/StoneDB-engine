# StoneDB-engine Architecture

## System Overview

```mermaid
graph TB
    CLI[CLI Interface]
    TXN[Transaction Manager]
    LOCK[Lock Manager]
    WAL[WAL Manager]
    STORAGE[Storage Manager]
    DISK[(Disk Files<br/>.sdb & .wal)]
    
    CLI -->|operations| TXN
    TXN -->|locks| LOCK
    TXN -->|logging| WAL
    TXN -->|data ops| STORAGE
    WAL -->|persist| DISK
    STORAGE -->|pages| DISK
    
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
    
    User->>CLI: put("key", "value")
    CLI->>TxnMgr: beginTransaction()
    TxnMgr->>WAL: log BEGIN_TXN
    WAL-->>TxnMgr: txnId
    TxnMgr-->>CLI: txnId
    
    CLI->>TxnMgr: putRecord(txnId, "key", "value")
    TxnMgr->>LockMgr: acquireLock(EXCLUSIVE)
    LockMgr-->>TxnMgr: granted
    TxnMgr->>WAL: log PUT_RECORD
    TxnMgr->>Storage: putRecord("key", "value")
    Storage-->>TxnMgr: success
    TxnMgr-->>CLI: success
    
    CLI->>TxnMgr: commitTransaction()
    TxnMgr->>WAL: log COMMIT_TXN
    TxnMgr->>Storage: flushAll()
    TxnMgr->>LockMgr: releaseAllLocks()
    TxnMgr-->>CLI: committed
```

## Storage Architecture

```mermaid
graph LR
    A[Record] -->|8 bytes header| B[Page 4KB]
    B -->|header + pages| C[Database File<br/>.sdb]
    
    D[Page Cache<br/>In-Memory] -->|evicts at 1000| B
    B -->|loads| D
    
    E[keyToPage Map] -->|O(1) lookup| B
    F[allocatedPages Set] -->|tracks| B
    
    style A fill:#ffe1e1
    style B fill:#e1f5ff
    style C fill:#f5f5f5
    style D fill:#fff4e1
    style E fill:#e1ffe1
    style F fill:#f0e1ff
```

**Storage Structure:**
- File header: 64 bytes
- Pages: 4KB each
- Record format: 2B keyLen + 2B valueLen + key + value
- Multi-page support with key-to-page mapping

## ACID Implementation

```mermaid
graph TB
    A[Atomicity] -->|WAL logs<br/>all operations| WAL
    C[Consistency] -->|Lock-based<br/>conflict prevention| LOCK
    I[Isolation] -->|Two-phase<br/>locking| LOCK
    D[Durability] -->|Flush to disk<br/>on commit| STORAGE
    
    WAL -->|replay on crash| WAL2[Crash Recovery]
    STORAGE -->|page flush| DISK2[(Persistent Storage)]
    LOCK -->|serializable| ISO2[Isolation Level]
    
    style A fill:#ffe1e1
    style C fill:#e1ffe1
    style I fill:#e1f5ff
    style D fill:#fff4e1
```

**ACID Properties:**
- **Atomicity**: All operations in transaction succeed or fail together via WAL
- **Consistency**: Validation prevents invalid states
- **Isolation**: Serializable isolation with two-phase locking
- **Durability**: Committed data flushed to disk

## Concurrency Control

```mermaid
graph TB
    T1[Transaction 1] -->|wants key A| L1[Lock Table]
    T2[Transaction 2] -->|wants key A| L1
    T3[Transaction 3] -->|wants key B| L1
    
    L1 -->|checks| DC[Deadlock Detection]
    DC -->|cycle found| ABORT[Abort T1]
    DC -->|no cycle| GRANT[Grant Lock]
    
    GRANT -->|SHARED| R1[Multiple Readers]
    GRANT -->|EXCLUSIVE| W1[Single Writer]
    
    style T1 fill:#ffe1e1
    style T2 fill:#e1ffe1
    style T3 fill:#e1f5ff
    style DC fill:#fff4e1
    style ABORT fill:#ff9999
    style GRANT fill:#99ff99
```

**Lock Management:**
- Shared locks: Multiple readers, no writers
- Exclusive locks: Single writer, blocks all others
- Deadlock detection: Cycle detection in wait graph
- Lock releases: All locks released on commit/abort

## Crash Recovery

```mermaid
sequenceDiagram
    participant Startup
    participant WAL
    participant Storage
    
    Startup->>WAL: open()
    WAL->>WAL: read all log entries
    WAL->>WAL: filter committed transactions
    WAL->>Storage: apply committed operations
    Storage->>Storage: restore all committed data
    Storage-->>Startup: recovery complete
```

**Recovery Process:**
1. On startup, read entire WAL file
2. Identify committed transactions (have COMMIT log entry)
3. Replay all PUT/DELETE operations from committed transactions
4. Apply operations to storage
5. Database restored to last consistent state

## Component Interactions

```mermaid
graph TB
    PUT[PUT Operation]
    GET[GET Operation]
    DEL[DELETE Operation]
    
    PUT --> TXN1[beginTransaction]
    GET --> TXN2[beginTransaction]
    DEL --> TXN3[beginTransaction]
    
    TXN1 --> LOCK1[acquire EXCLUSIVE]
    TXN2 --> LOCK2[acquire SHARED]
    TXN3 --> LOCK3[acquire EXCLUSIVE]
    
    LOCK1 --> WAL1[log PUT_RECORD]
    LOCK2 --> WAL2[log GET]
    LOCK3 --> WAL3[log DELETE_RECORD]
    
    WAL1 --> STOR1[store to page]
    WAL2 --> STOR2[read from page]
    WAL3 --> STOR3[mark deleted]
    
    STOR1 --> COMMIT1[commit]
    STOR2 --> COMMIT2[commit]
    STOR3 --> COMMIT3[commit]
    
    COMMIT1 --> FLUSH1[flush pages]
    COMMIT2 --> RELEASE2[release locks]
    COMMIT3 --> FLUSH3[flush pages]
```

## Data Flow

**Write Path:**
1. User command → CLI
2. Transaction begin → WAL log
3. Acquire exclusive lock
4. Log operation in WAL
5. Write to storage (in-memory cache)
6. On commit: flush WAL, flush pages, release locks

**Read Path:**
1. User command → CLI
2. Transaction begin → WAL log
3. Acquire shared lock
4. Read from storage cache (or disk)
5. Return value
6. On commit: release locks

**Delete Path:**
1. Similar to write path
2. Record marked deleted (keyLen = 0, valueLen preserved)
3. Removed from keyToPage map
4. Space reusable on next allocation

## Page Management

```mermaid
graph LR
    A[putRecord] -->|check| B[keyToPage exists?]
    B -->|yes| C[update in-place]
    B -->|no| D[find free space]
    D -->|found| E[write to page]
    D -->|not found| F[allocateNewPage]
    F --> G[track in allocatedPages]
    G --> E
    E --> H[update keyToPage]
    
    I[getRecord] -->|lookup| J[keyToPage map]
    J -->|found| K[read from page]
    J -->|not found| L[scan all pages]
    L --> K
    
    style A fill:#ffe1e1
    style F fill:#e1f5ff
    style J fill:#e1ffe1
```

## WAL Structure

```mermaid
graph TB
    A[WAL File Header<br/>32 bytes] --> B[Log Entry 1]
    B --> C[Log Entry 2]
    C --> D[Log Entry N]
    
    B --> E[Entry Format:<br/>Type + TxnID + Timestamp<br/>+ KeyLen + Key<br/>+ ValueLen + Value]
    
    F[Log Types] --> G[BEGIN_TXN]
    F --> H[COMMIT_TXN]
    F --> I[ABORT_TXN]
    F --> J[PUT_RECORD]
    F --> K[DELETE_RECORD]
    
    style A fill:#f5f5f5
    style E fill:#e1f5ff
    style F fill:#fff4e1
```

## Memory Management

```mermaid
graph TB
    A[Page Cache<br/>unordered_map] -->|max 1000 pages| B[LRU Eviction]
    B -->|evict non-dirty| C[Remove from cache]
    B -->|all dirty| D[Evict oldest]
    
    E[Smart Pointers] -->|shared_ptr| A
    E -->|automatic cleanup| F[RAII]
    
    style A fill:#e1f5ff
    style B fill:#fff4e1
    style E fill:#e1ffe1
```

**Memory Policies:**
- Page cache limited to 1000 pages (4MB max)
- LRU eviction: prefer non-dirty pages
- Smart pointers for automatic memory management
- RAII for resource cleanup

## Performance Optimizations

**Current Optimizations:**
- Page caching reduces disk I/O
- keyToPage map for O(1) lookups
- Multi-page allocation prevents single-page bottleneck
- LRU eviction controls memory usage
- Batch WAL writes for durability

**Future Optimizations:**
- B-tree indexing (Issue #12)
- MVCC for better concurrency (Issue #13)
- Page compression (Issue #14)
- WAL checkpointing (Issue #15)
