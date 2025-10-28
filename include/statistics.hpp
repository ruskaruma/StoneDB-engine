#pragma once
#include<cstdint>
#include<atomic>
#include<mutex>

namespace stonedb
{
    //Statistics tracking class (GitHub Issue #9)
    class Statistics
    {
    private:
        std::atomic<uint64_t> transactionCount;
        std::atomic<uint64_t> putOperations;
        std::atomic<uint64_t> getOperations;
        std::atomic<uint64_t> deleteOperations;
        std::atomic<uint64_t> cacheHits;
        std::atomic<uint64_t> cacheMisses;
        std::atomic<uint64_t> lockWaits;
        std::atomic<uint64_t> deadlocksDetected;
        
    public:
        Statistics();
        
        void incrementTransactions() { transactionCount++; }
        void incrementPutOps() { putOperations++; }
        void incrementGetOps() { getOperations++; }
        void incrementDeleteOps() { deleteOperations++; }
        void incrementCacheHits() { cacheHits++; }
        void incrementCacheMisses() { cacheMisses++; }
        void incrementLockWaits() { lockWaits++; }
        void incrementDeadlocks() { deadlocksDetected++; }
        
        uint64_t getTransactionCount() const { return transactionCount.load(); }
        uint64_t getPutOps() const { return putOperations.load(); }
        uint64_t getGetOps() const { return getOperations.load(); }
        uint64_t getDeleteOps() const { return deleteOperations.load(); }
        uint64_t getCacheHits() const { return cacheHits.load(); }
        uint64_t getCacheMisses() const { return cacheMisses.load(); }
        uint64_t getLockWaits() const { return lockWaits.load(); }
        uint64_t getDeadlocks() const { return deadlocksDetected.load(); }
        
        double getCacheHitRatio() const;
        void printStatistics() const;
        void reset();
    };
}

