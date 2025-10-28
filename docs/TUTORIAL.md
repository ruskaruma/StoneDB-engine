# Building a Database Tutorial

This tutorial explains how each component of StoneDB-engine was built, step by step.

## Part 1: Storage Layer

### Introduction
The storage layer manages persistent data using a page-based architecture. Each page is 4KB, and records are stored sequentially within pages.

### Step 1: Page Structure
```cpp
struct Page {
    PageId pageId;
    std::vector<uint8_t> data;  // 4KB of raw data
    bool isDirty;
};
```
Pages are stored on disk with a header (64 bytes) followed by pages. Each record has a header: 2 bytes keyLen + 2 bytes valueLen.

### Step 2: Record Format
Records are stored as:
- 2 bytes: key length
- 2 bytes: value length  
- N bytes: key data
- M bytes: value data

When a record is deleted, keyLen is set to 0, but valueLen is preserved to allow skipping during scans.

### Step 3: Multi-page Support
When page 0 fills up, we allocate new pages:
1. Track allocated pages in `allocatedPages` set
2. Map keys to page IDs in `keyToPage` for O(1) lookups
3. Reuse free pages when possible

### Step 4: Page Caching
Pages are cached in memory (`pageCache`) to avoid disk I/O. When cache exceeds 1000 pages, we evict non-dirty pages using LRU.

## Part 2: Write-Ahead Logging (WAL)

### Purpose
WAL ensures durability: all operations are logged before being applied to storage.

### Implementation
1. Every operation writes a log entry first
2. Log entries are serialized: type, txnId, timestamp, keyLen, key, valueLen, value
3. On commit, WAL is flushed to disk
4. On crash, WAL is replayed to recover committed transactions

### Checkpointing
After checkpoint:
1. Flush all dirty pages to storage
2. Flush WAL
3. Truncate WAL file (all committed data is now in storage)

## Part 3: Lock Manager

### Purpose
Ensures transaction isolation by controlling concurrent access.

### Lock Types
- **SHARED**: Multiple readers can access simultaneously
- **EXCLUSIVE**: Only one writer, blocks all others

### Implementation
1. `lockTable` maps keys to list of lock requests
2. Transactions acquire locks before operations
3. Deadlock detection uses cycle detection in wait graph
4. When transaction commits/aborts, all locks are released

### Deadlock Detection
Uses DFS to detect cycles in the wait graph:
- Transaction A waits for key locked by Transaction B
- Transaction B waits for key locked by Transaction A
- = Deadlock!

## Part 4: Transaction Management

### ACID Properties

**Atomicity**: All operations in a transaction succeed or fail together
- Implemented via WAL: if commit fails, operations aren't persisted

**Consistency**: Database remains in valid state
- Validation of key/value sizes before operations

**Isolation**: Transactions don't interfere
- Lock-based: shared locks for reads, exclusive for writes
- Serializable isolation level

**Durability**: Committed changes persist
- WAL is flushed on commit
- Storage pages are flushed

### Transaction Lifecycle
1. `beginTransaction()` - Allocate ID, log BEGIN
2. Operations - Acquire locks, log operations, modify storage
3. `commitTransaction()` - Flush WAL, flush storage, release locks
4. `abortTransaction()` - Log ABORT, release locks (storage changes remain but are overwritten)

## Part 4: Putting It All Together

### Example: PUT operation
1. Begin transaction
2. Acquire exclusive lock on key
3. Log PUT to WAL
4. Store record in storage (allocating page if needed)
5. Commit transaction
6. Release locks

### Crash Recovery
1. On startup, open storage and WAL
2. Read all WAL entries
3. Identify committed transactions (have COMMIT log entry)
4. Replay all PUT/DELETE operations from committed transactions
5. Apply to storage

This ensures that committed transactions are not lost even if the system crashes before storage is flushed.

## Key Design Decisions

1. **Page-based storage**: Efficient for disk I/O, easy to cache
2. **WAL before storage**: Ensures durability without blocking
3. **Lock-based concurrency**: Simple to implement, deadlock detection prevents hangs
4. **Immediate write to storage**: Simpler than deferred writes, good for small datasets
5. **keyToPage mapping**: Fast lookups without full scan

## Future Improvements

- B-tree index for faster lookups (Issue #12)
- MVCC for better concurrency (Issue #13)
- Page compression to save space (Issue #14)
- More sophisticated cache eviction (currently simple LRU)

