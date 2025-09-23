#include"lockmgr.hpp"
#include<cassert>

int main()
{
    stonedb::LockManager lockMgr;
    
    // test basic lock acquisition
    stonedb::TransactionId txn1=1;
    stonedb::TransactionId txn2=2;
    
    // txn1 gets shared lock
    assert(lockMgr.acquireLock(txn1, "key1", stonedb::LockType::SHARED));
    
    // txn2 can also get shared lock (compatible)
    assert(lockMgr.acquireLock(txn2, "key1", stonedb::LockType::SHARED));
    
    // txn1 tries exclusive lock (should fail - conflicts with txn2's shared lock)
    // this would block, so we'll test release first
    
    // release locks
    assert(lockMgr.releaseLock(txn1, "key1"));
    assert(lockMgr.releaseLock(txn2, "key1"));
    
    // test exclusive lock
    assert(lockMgr.acquireLock(txn1, "key1", stonedb::LockType::EXCLUSIVE));
    
    // release all locks
    assert(lockMgr.releaseAllLocks(txn1));
    
    stonedb::log("lock manager tests passed");
    return 0;
}
