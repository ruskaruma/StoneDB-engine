#pragma once
#include"common.hpp"
#include<unordered_map>
#include<unordered_set>
#include<mutex>
#include<condition_variable>

namespace stonedb
{
    struct LockRequest 
    {
        TransactionId txnId;
        LockType type;
        bool granted;
        
        LockRequest(TransactionId id, LockType t) : txnId(id), type(t), granted(false) {}
    };
    
    class LockManager
    {
    private:
        std::mutex lockMutex;
        std::condition_variable lockCondition;
        std::unordered_map<std::string, std::vector<LockRequest>> lockTable;
        std::unordered_map<TransactionId, std::unordered_set<std::string>> txnLocks;
        std::unordered_map<TransactionId, std::unordered_set<std::string>> txnWaits;
        
        bool canGrantLock(const std::string& key, LockType requestedType);
        void grantLock(const std::string& key, TransactionId txnId, LockType type);
        void releaseLock(const std::string& key, TransactionId txnId);
        bool hasDeadlock(TransactionId txnId);
        
    public:
        LockManager();
        ~LockManager();
        
        bool acquireLock(TransactionId txnId, const std::string& key, LockType type);
        bool releaseLock(TransactionId txnId, const std::string& key);
        bool releaseAllLocks(TransactionId txnId);
        void printLockStatus();
    };
}
