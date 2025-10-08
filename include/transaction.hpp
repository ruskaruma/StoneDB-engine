#pragma once
#include"common.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<memory>

namespace stonedb
{
    enum class TransactionState
    {
        ACTIVE,
        COMMITTED,
        ABORTED
    };
    struct Transaction
    {
        TransactionId txnId;
        TransactionState state;
        std::unordered_set<std::string> readSet;
        std::unordered_set<std::string> writeSet;    
        Transaction(TransactionId id) : txnId(id), state(TransactionState::ACTIVE) {}
    };
    class TransactionManager
    {
    private:
        std::shared_ptr<StorageManager> storage;
        std::shared_ptr<WALManager> wal;
        std::shared_ptr<LockManager> lockMgr;
        std::unordered_map<TransactionId, Transaction> activeTxns;
        TransactionId nextTxnId;
        std::mutex txnMutex;  
        bool acquireLocks(TransactionId txnId, const std::string& key, bool isWrite);
        void releaseLocks(TransactionId txnId);
        
    public:
        TransactionManager(std::shared_ptr<StorageManager> storage,
                          std::shared_ptr<WALManager> wal,
                          std::shared_ptr<LockManager> lockMgr);
        TransactionId beginTransaction();
        bool commitTransaction(TransactionId txnId);
        bool abortTransaction(TransactionId txnId);
        bool putRecord(TransactionId txnId, const std::string& key, const std::string& value);
        bool getRecord(TransactionId txnId, const std::string& key, std::string& value);
        bool deleteRecord(TransactionId txnId, const std::string& key);

        void printTransactionStatus();
    };
}
