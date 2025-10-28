#include"statistics.hpp"
#include"common.hpp"

namespace stonedb
{
    Statistics::Statistics() 
        : transactionCount(0), putOperations(0), getOperations(0), 
          deleteOperations(0), cacheHits(0), cacheMisses(0), 
          lockWaits(0), deadlocksDetected(0)
    {
    }
    
    double Statistics::getCacheHitRatio() const
    {
        uint64_t hits=cacheHits.load();
        uint64_t misses=cacheMisses.load();
        uint64_t total=hits + misses;
        if(total == 0) return 0.0;
        return (static_cast<double>(hits) / static_cast<double>(total)) * 100.0;
    }
    
    void Statistics::printStatistics() const
    {
        log("=== Database Statistics ===");
        log("Transactions: " + std::to_string(transactionCount.load()));
        log("PUT operations: " + std::to_string(putOperations.load()));
        log("GET operations: " + std::to_string(getOperations.load()));
        log("DELETE operations: " + std::to_string(deleteOperations.load()));
        log("Cache hits: " + std::to_string(cacheHits.load()));
        log("Cache misses: " + std::to_string(cacheMisses.load()));
        log("Cache hit ratio: " + std::to_string(getCacheHitRatio()) + "%");
        log("Lock waits: " + std::to_string(lockWaits.load()));
        log("Deadlocks detected: " + std::to_string(deadlocksDetected.load()));
    }
    
    void Statistics::reset()
    {
        transactionCount=0;
        putOperations=0;
        getOperations=0;
        deleteOperations=0;
        cacheHits=0;
        cacheMisses=0;
        lockWaits=0;
        deadlocksDetected=0;
    }
}

