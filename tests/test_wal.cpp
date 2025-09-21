#include"wal.hpp"
#include<cassert>
#include<iostream>

int main()
{
    stonedb::WALManager wal;
    
    // test basic operations
    assert(wal.open("test.wal"));
    assert(wal.isOpen());
    
    // test transaction logging
    stonedb::TransactionId txn1=1;
    assert(wal.logBeginTxn(txn1));
    assert(wal.logPutRecord(txn1, "key1", "value1"));
    assert(wal.logPutRecord(txn1, "key2", "value2"));
    assert(wal.logCommitTxn(txn1));
    
    // test abort
    stonedb::TransactionId txn2=2;
    assert(wal.logBeginTxn(txn2));
    assert(wal.logPutRecord(txn2, "key3", "value3"));
    assert(wal.logAbortTxn(txn2));
    
    // test replay
    auto entries=wal.replayLog();
    std::cout << "Found " << entries.size() << " committed operations" << std::endl;
    
    // for now, just check that we can replay without crashing
    wal.close();
    stonedb::log("wal tests passed");
    return 0;
}
