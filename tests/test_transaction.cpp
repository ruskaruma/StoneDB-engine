#include"transaction.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<cassert>

int main()
{
    // create components
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    // initialize
    assert(storage->open("test_db.sdb"));
    assert(wal->open("test.wal"));
    
    // create transaction manager
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    // test transaction lifecycle
    auto txnId=txnMgr.beginTransaction();
    assert(txnId != stonedb::INVALID_TXN_ID);
    
    // test operations
    assert(txnMgr.putRecord(txnId, "key1", "value1"));
    assert(txnMgr.putRecord(txnId, "key2", "value2"));
    
    std::string value;
    assert(txnMgr.getRecord(txnId, "key1", value));
    assert(value == "value1");
    
    // commit transaction
    assert(txnMgr.commitTransaction(txnId));
    
    // cleanup
    storage->close();
    wal->close();
    
    stonedb::log("transaction manager tests passed");
    return 0;
}
