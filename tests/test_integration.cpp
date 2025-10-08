#include"transaction.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<iostream>
#include<cassert>

int main()
{
    std::cout << "StoneDB Integration Test" << std::endl;
    
    // create components
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    // initialize database
    assert(storage->open("integration_test.sdb"));
    assert(wal->open("integration_test.wal"));
    
    // create transaction manager
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    // test basic operations
    std::cout << "Testing basic operations..." << std::endl;
    
    auto txn1=txnMgr.beginTransaction();
    assert(txn1 != stonedb::INVALID_TXN_ID);
    
    // test put operations
    assert(txnMgr.putRecord(txn1, "user1", "alice"));
    assert(txnMgr.putRecord(txn1, "user2", "bob"));
    assert(txnMgr.putRecord(txn1, "user3", "charlie"));
    
    // test get operations
    std::string value;
    assert(txnMgr.getRecord(txn1, "user1", value));
    assert(value == "alice");
    
    assert(txnMgr.getRecord(txn1, "user2", value));
    assert(value == "bob");
    
    // commit transaction
    assert(txnMgr.commitTransaction(txn1));
    
    // test another transaction
    auto txn2=txnMgr.beginTransaction();
    assert(txn2 != stonedb::INVALID_TXN_ID);
    
    // update a record
    assert(txnMgr.putRecord(txn2, "user1", "alice_updated"));
    
    // delete a record
    assert(txnMgr.deleteRecord(txn2, "user3"));
    
    // commit transaction
    assert(txnMgr.commitTransaction(txn2));
    
    // test final state
    auto txn3=txnMgr.beginTransaction();
    assert(txn3 != stonedb::INVALID_TXN_ID);
    
    // check updated record
    assert(txnMgr.getRecord(txn3, "user1", value));
    assert(value == "alice_updated");
    
    // check deleted record
    assert(!txnMgr.getRecord(txn3, "user3", value));
    
    // check remaining record
    assert(txnMgr.getRecord(txn3, "user2", value));
    assert(value == "bob");
    
    assert(txnMgr.commitTransaction(txn3));
    
    // test scan functionality
    std::cout << "Testing scan functionality..." << std::endl;
    auto records=storage->scanRecords();
    assert(records.size() == 2); // user1 and user2 should exist
    
    bool foundUser1=false, foundUser2=false;
    for(const auto& record : records) {
        if(record.key == "user1" && record.value == "alice_updated") {
            foundUser1=true;
        }
        if(record.key == "user2" && record.value == "bob") {
            foundUser2=true;
        }
    }
    assert(foundUser1);
    assert(foundUser2);
    
    // cleanup
    storage->close();
    wal->close();
    
    // remove test files
    std::remove("integration_test.sdb");
    std::remove("integration_test.wal");
    
    std::cout << "Integration test passed" << std::endl;
    return 0;
}
