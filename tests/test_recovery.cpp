#include"transaction.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<cassert>
#include<fstream>

int main()
{
    // test crash recovery scenario
    std::cout << "Testing crash recovery..." << std::endl;
    
    // create components
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    // initialize database
    assert(storage->open("recovery_test.sdb"));
    assert(wal->open("recovery_test.wal"));
    
    // create transaction manager
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    // simulate some transactions
    auto txn1=txnMgr.beginTransaction();
    assert(txnMgr.putRecord(txn1, "key1", "value1"));
    assert(txnMgr.putRecord(txn1, "key2", "value2"));
    assert(txnMgr.commitTransaction(txn1));
    
    auto txn2=txnMgr.beginTransaction();
    assert(txnMgr.putRecord(txn2, "key3", "value3"));
    // simulate crash before commit - don't commit txn2
    
    // close database to simulate crash
    storage->close();
    wal->close();
    
    std::cout << "Simulated crash - reopening database..." << std::endl;
    
    // reopen database
    auto storage2=std::make_shared<stonedb::StorageManager>();
    auto wal2=std::make_shared<stonedb::WALManager>();
    
    assert(storage2->open("recovery_test.sdb"));
    assert(wal2->open("recovery_test.wal"));
    
    // test recovery - should only have committed transactions
    std::string value;
    assert(storage2->getRecord("key1", value));
    assert(value == "value1");
    
    assert(storage2->getRecord("key2", value));
    assert(value == "value2");
    
    // key3 should not exist (transaction was not committed)
    assert(!storage2->getRecord("key3", value));
    
    std::cout << "Recovery test passed - only committed data recovered" << std::endl;
    
    // test WAL replay
    std::cout << "Testing WAL replay..." << std::endl;
    auto entries=wal2->replayLog();
    std::cout << "Found " << entries.size() << " log entries" << std::endl;
    
    // cleanup
    storage2->close();
    wal2->close();
    
    // remove test files
    std::remove("recovery_test.sdb");
    std::remove("recovery_test.wal");
    
    std::cout << "Recovery tests passed" << std::endl;
    return 0;
}
