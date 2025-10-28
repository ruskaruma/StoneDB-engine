#include"transaction.hpp"

namespace stonedb
{
    TransactionManager::TransactionManager(std::shared_ptr<StorageManager> storage,
                                         std::shared_ptr<WALManager> wal,
                                         std::shared_ptr<LockManager> lockMgr)
        : storage(storage), wal(wal), lockMgr(lockMgr), nextTxnId(1)
    {
    }
    
    TransactionId TransactionManager::beginTransaction()
    {
        std::lock_guard<std::mutex> lock(txnMutex);
        
        //fix bug #19: handle transaction ID wraparound
        TransactionId txnId=nextTxnId++;
        if(txnId == INVALID_TXN_ID)
        {
            //wrapped around - skip INVALID_TXN_ID and continue
            txnId=nextTxnId++;
        }
        
        activeTxns.emplace(txnId, Transaction(txnId));
        if(!wal->logBeginTxn(txnId))
        {
            logError("failed to log transaction begin");
            return INVALID_TXN_ID;
        }
        
        log("started transaction " + std::to_string(txnId));
        return txnId;
    }
    
    bool TransactionManager::commitTransaction(TransactionId txnId)
    {
        std::lock_guard<std::mutex> lock(txnMutex);
        
        auto it=activeTxns.find(txnId);
        if(it == activeTxns.end())
        {
            logError("transaction " + std::to_string(txnId) + " not found");
            return false;
        }
        
        Transaction& txn=it->second;
        if(txn.state != TransactionState::ACTIVE)
        {
            logError("transaction " + std::to_string(txnId) + " not active");
            return false;
        }
        

        if(!wal->logCommitTxn(txnId))
        {
            logError("failed to log transaction commit");
            return false;
        }
        
        
        if(!storage->flushAll())
        {
            logError("failed to flush storage");
            return false;
        }
        releaseLocks(txnId);
        txn.state=TransactionState::COMMITTED;
        activeTxns.erase(it);
        log("committed transaction " + std::to_string(txnId));
        return true;
    }
    bool TransactionManager::abortTransaction(TransactionId txnId)
    {
        std::lock_guard<std::mutex> lock(txnMutex);
        
        auto it=activeTxns.find(txnId);
        if(it == activeTxns.end())
        {
            logError("transaction " + std::to_string(txnId) + " not found");
            return false;
        }
        Transaction& txn=it->second;
        if(txn.state != TransactionState::ACTIVE)
        {
            logError("transaction " + std::to_string(txnId) + " not active");
            return false;
        }
        if(!wal->logAbortTxn(txnId))
        {
            logError("failed to log transaction abort");
            return false;
        }
        releaseLocks(txnId);
        txn.state=TransactionState::ABORTED;
        activeTxns.erase(it);
        
        log("aborted transaction " + std::to_string(txnId));
        return true;
    }
    bool TransactionManager::putRecord(TransactionId txnId, const std::string& key, const std::string& value)
    {
        {
            std::lock_guard<std::mutex> lock(txnMutex);
            
            auto it=activeTxns.find(txnId);
            if(it == activeTxns.end()) {
                logError("transaction " + std::to_string(txnId) + " not found");
                return false;
            }
            
            Transaction& txn=it->second;
            if(txn.state != TransactionState::ACTIVE) {
                logError("transaction " + std::to_string(txnId) + " not active");
                return false;
            }
        }
        
        if(!acquireLocks(txnId, key, true))
        {
            logError("failed to acquire lock for " + key);
            return false;
        }
        if(!wal->logPutRecord(txnId, key, value))
        {
            logError("failed to log put operation");
            return false;
        }
        if(!storage->putRecord(key, value))
        {
            logError("failed to put record");
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(txnMutex);
            auto it=activeTxns.find(txnId);
            if(it != activeTxns.end()) {
                it->second.writeSet.insert(key);
            }
        }
        log("txn " + std::to_string(txnId) + " put " + key + " = " + value);
        return true;
    }
    bool TransactionManager::getRecord(TransactionId txnId, const std::string& key, std::string& value)
    {
        {
            std::lock_guard<std::mutex> lock(txnMutex);
            
            auto it=activeTxns.find(txnId);
            if(it == activeTxns.end())
            {
                logError("transaction " + std::to_string(txnId) + " not found");
                return false;
            }
            Transaction& txn=it->second;
            if(txn.state != TransactionState::ACTIVE)
            {
                logError("transaction " + std::to_string(txnId) + " not active");
                return false;
            }
        }
        // acquire shared lock (without holding txnMutex)
        if(!acquireLocks(txnId, key, false))
        {
            logError("failed to acquire lock for " + key);
            return false;
        }
        

        bool found=storage->getRecord(key, value);
        if(found){
            {
                std::lock_guard<std::mutex> lock(txnMutex);
                auto it=activeTxns.find(txnId);
                if(it != activeTxns.end()) {
                    it->second.readSet.insert(key);
                }
            }
            log("txn " + std::to_string(txnId) + " get " + key + " = " + value);
        }
        else
        {
            log("txn " + std::to_string(txnId) + " get " + key + " (not found)");
        }
        
        return found;
    }
    
    bool TransactionManager::deleteRecord(TransactionId txnId, const std::string& key)
    {
        {
            std::lock_guard<std::mutex> lock(txnMutex);
            
            auto it=activeTxns.find(txnId);
            if(it == activeTxns.end())
            {
                logError("transaction " + std::to_string(txnId) + " not found");
                return false;
            }
            
            Transaction& txn=it->second;
            if(txn.state != TransactionState::ACTIVE)
            {
                logError("transaction " + std::to_string(txnId) + " not active");
                return false;
            }
        }
        
        //fix bug #10: release txnMutex before acquiring locks to avoid deadlock
        if(!acquireLocks(txnId, key, true))
        {
            logError("failed to acquire lock for " + key);
            return false;
        }
        
        {
            std::lock_guard<std::mutex> lock(txnMutex);
            auto it=activeTxns.find(txnId);
            if(it == activeTxns.end() || it->second.state != TransactionState::ACTIVE)
            {
                releaseLocks(txnId);
                return false;
            }
            
            if(!wal->logDeleteRecord(txnId, key))
            {
                logError("failed to log delete operation");
                releaseLocks(txnId);
                return false;
            }
            if(!storage->deleteRecord(key))
            {
                logError("failed to delete record");
                releaseLocks(txnId);
                return false;
            }
            it->second.writeSet.insert(key);
            log("txn " + std::to_string(txnId) + " delete " + key);
            return true;
        }
    }
    bool TransactionManager::acquireLocks(TransactionId txnId, const std::string& key, bool isWrite)
    {
        LockType lockType=isWrite ? LockType::EXCLUSIVE : LockType::SHARED;
        return lockMgr->acquireLock(txnId, key, lockType);
    }
    
    void TransactionManager::releaseLocks(TransactionId txnId)
    {
        lockMgr->releaseAllLocks(txnId);
    }
    void TransactionManager::printTransactionStatus()
    {
        std::lock_guard<std::mutex> lock(txnMutex);
        
        log("=== Transaction Status ===");
        for(const auto& pair : activeTxns) {
            const Transaction& txn=pair.second;
            log("Txn " + std::to_string(txn.txnId) + 
                " state: " + (txn.state == TransactionState::ACTIVE ? "ACTIVE" : "INACTIVE"));
        }
    }
}
