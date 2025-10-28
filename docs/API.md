# StoneDB-engine API Documentation

## Overview

StoneDB-engine provides a C++ API for an ACID-compliant embedded database. All classes are in the `stonedb` namespace.

## StorageManager

The `StorageManager` class handles page-based storage and caching.

### Methods

#### `bool open(const std::string& path)`
Opens or creates a database file at the specified path.
- **Parameters**: `path` - Path to database file (.sdb)
- **Returns**: `true` on success, `false` on failure
- **Example**:
```cpp
auto storage = std::make_shared<stonedb::StorageManager>();
if(!storage->open("mydb.sdb")) {
    // handle error
}
```

#### `void close()`
Closes the database and flushes all pages to disk.
- **Example**:
```cpp
storage->close();
```

#### `bool isOpen() const`
Checks if the database is currently open.
- **Returns**: `true` if open, `false` otherwise

#### `bool putRecord(const std::string& key, const std::string& value)`
Stores a key-value pair in the database.
- **Parameters**: 
  - `key` - Record key (max 255 bytes)
  - `value` - Record value (max 1MB)
- **Returns**: `true` on success, `false` on failure
- **Example**:
```cpp
if(storage->putRecord("name", "Alice")) {
    // success
}
```

#### `bool getRecord(const std::string& key, std::string& value)`
Retrieves a value by key.
- **Parameters**:
  - `key` - Record key to search for
  - `value` - Output parameter for the value
- **Returns**: `true` if found, `false` if not found
- **Example**:
```cpp
std::string value;
if(storage->getRecord("name", value)) {
    std::cout << "Value: " << value << std::endl;
}
```

#### `bool deleteRecord(const std::string& key)`
Deletes a record by key.
- **Parameters**: `key` - Record key to delete
- **Returns**: `true` on success, `false` if not found
- **Example**:
```cpp
storage->deleteRecord("temp");
```

#### `std::vector<Record> scanRecords()`
Retrieves all records in the database.
- **Returns**: Vector of `Record` structs
- **Example**:
```cpp
auto records = storage->scanRecords();
for(const auto& record : records) {
    std::cout << record.key << " = " << record.value << std::endl;
}
```

#### `bool flushAll()`
Flushes all dirty pages to disk.
- **Returns**: `true` on success
- **Example**:
```cpp
storage->flushAll();
```

## TransactionManager

The `TransactionManager` class manages ACID transactions.

### Constructor

```cpp
TransactionManager(
    std::shared_ptr<StorageManager> storage,
    std::shared_ptr<WALManager> wal,
    std::shared_ptr<LockManager> lockMgr
);
```

### Methods

#### `TransactionId beginTransaction()`
Starts a new transaction.
- **Returns**: Transaction ID, or `INVALID_TXN_ID` on failure
- **Example**:
```cpp
auto txnId = txnMgr.beginTransaction();
```

#### `bool commitTransaction(TransactionId txnId)`
Commits a transaction.
- **Parameters**: `txnId` - Transaction ID to commit
- **Returns**: `true` on success
- **Example**:
```cpp
if(txnMgr.commitTransaction(txnId)) {
    // committed
}
```

#### `bool abortTransaction(TransactionId txnId)`
Aborts a transaction, rolling back changes.
- **Parameters**: `txnId` - Transaction ID to abort
- **Returns**: `true` on success

#### `bool putRecord(TransactionId txnId, const std::string& key, const std::string& value)`
Stores a record within a transaction.
- **Parameters**: 
  - `txnId` - Active transaction ID
  - `key` - Record key
  - `value` - Record value
- **Returns**: `true` on success

#### `bool getRecord(TransactionId txnId, const std::string& key, std::string& value)`
Retrieves a record within a transaction.
- **Returns**: `true` if found

#### `bool deleteRecord(TransactionId txnId, const std::string& key)`
Deletes a record within a transaction.
- **Returns**: `true` on success

### Example Usage

```cpp
auto storage = std::make_shared<stonedb::StorageManager>();
auto wal = std::make_shared<stonedb::WALManager>();
auto lockMgr = std::make_shared<stonedb::LockManager>();

storage->open("mydb.sdb");
wal->open("mydb.wal");

stonedb::TransactionManager txnMgr(storage, wal, lockMgr);

// Transaction example
auto txnId = txnMgr.beginTransaction();
txnMgr.putRecord(txnId, "key1", "value1");
txnMgr.putRecord(txnId, "key2", "value2");
if(txnMgr.commitTransaction(txnId)) {
    // all changes committed
} else {
    txnMgr.abortTransaction(txnId);
    // changes rolled back
}
```

## WALManager

The `WALManager` class handles write-ahead logging for crash recovery.

### Methods

#### `bool open(const std::string& path)`
Opens or creates a WAL file.
- **Parameters**: `path` - Path to WAL file (.wal)

#### `void close()`
Closes the WAL file and flushes.

#### `bool flush()`
Flushes WAL entries to disk.

#### `bool checkpoint(std::shared_ptr<StorageManager> storage)`
Performs a checkpoint, flushing all pages and WAL.
- **Parameters**: `storage` - StorageManager instance to flush

#### `bool truncateLog()`
Truncates the WAL file after checkpoint.

#### `std::vector<LogEntry> replayLog()`
Replays log entries for crash recovery.
- **Returns**: Vector of committed log entries

## LockManager

The `LockManager` class handles concurrency control.

### Methods

#### `bool acquireLock(TransactionId txnId, const std::string& key, LockType type)`
Acquires a lock on a key.
- **Parameters**:
  - `txnId` - Transaction ID
  - `key` - Key to lock
  - `type` - `LockType::SHARED` or `LockType::EXCLUSIVE`
- **Returns**: `true` on success, `false` on deadlock

#### `bool releaseLock(TransactionId txnId, const std::string& key)`
Releases a lock on a key.

#### `bool releaseAllLocks(TransactionId txnId)`
Releases all locks for a transaction.

## Statistics

The `Statistics` class tracks database operation metrics (GitHub Issue #9).

### Methods

#### `void incrementTransactions()`
Increments transaction counter.

#### `void incrementPutOps()`, `incrementGetOps()`, `incrementDeleteOps()`
Increment operation counters.

#### `void incrementCacheHits()`, `incrementCacheMisses()`
Increment cache statistics.

#### `double getCacheHitRatio() const`
Returns cache hit ratio as percentage.

#### `void printStatistics() const`
Prints all statistics using log().

#### `void reset()`
Resets all counters to zero.

### Example

```cpp
stonedb::Statistics stats;
// ... perform operations ...
stats.printStatistics();
```

## Error Handling

All methods return `bool` for success/failure. The `ErrorCode` enum is available for future use (GitHub Issue #7).

## Constants

- `PAGE_SIZE`: 4096 bytes
- `MAX_KEY_SIZE`: 255 bytes
- `MAX_VALUE_SIZE`: 1MB
- `INVALID_TXN_ID`: 0

