#include"lockmgr.hpp"
#include<cassert>

int main()
{
    stonedb::LockManager lockMgr;
    
    // test basic lock acquisition
    stonedb::TransactionId txn1=1;
    
    // txn1 gets shared lock
    assert(lockMgr.acquireLock(txn1, "key1", stonedb::LockType::SHARED));
    
    // release lock
    assert(lockMgr.releaseLock(txn1, "key1"));
    
    // test exclusive lock
    assert(lockMgr.acquireLock(txn1, "key1", stonedb::LockType::EXCLUSIVE));
    
    // release all locks
    assert(lockMgr.releaseAllLocks(txn1));
    
    stonedb::log("lock manager tests passed");
    return 0;
}
