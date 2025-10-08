#include"lockmgr.hpp"
#include<algorithm>
#include<functional>

namespace stonedb
{
    LockManager::LockManager()
    {
    }
    
    LockManager::~LockManager()
    {
    }
    
    bool LockManager::canGrantLock(const std::string& key, LockType requestedType)
    {
        auto it=lockTable.find(key);
        if(it == lockTable.end()) return true;
        
        const auto& requests=it->second;
        
        for(const auto& req : requests) {
            if(req.granted) {
                // shared locks are compatible with each other
                if(requestedType == LockType::SHARED && req.type == LockType::SHARED) {
                    continue;
                }
                // any other combination conflicts
                return false;
            }
        }
        
        return true;
    }
    
    void LockManager::grantLock(const std::string& key, TransactionId txnId, LockType type)
    {
        auto& requests=lockTable[key];
        
        // find the request and grant it
        for(auto& req : requests) {
            if(req.txnId == txnId && req.type == type && !req.granted) {
                req.granted=true;
                txnLocks[txnId].insert(key);
                txnWaits[txnId].erase(key);
                break;
            }
        }
    }
    
    void LockManager::releaseLock(const std::string& key, TransactionId txnId)
    {
        auto it=lockTable.find(key);
        if(it == lockTable.end()) return;
        
        auto& requests=it->second;
        
        // remove the granted lock
        requests.erase(std::remove_if(requests.begin(), requests.end(),
            [txnId](const LockRequest& req) {
                return req.txnId == txnId && req.granted;
            }), requests.end());
        
        // clean up empty entries
        if(requests.empty()) {
            lockTable.erase(it);
        }
        
        txnLocks[txnId].erase(key);
    }
    
    bool LockManager::acquireLock(TransactionId txnId, const std::string& key, LockType type)
    {
        std::unique_lock<std::mutex> lock(lockMutex);
        
        // check if transaction already holds a lock on this key
        auto txnIt=txnLocks.find(txnId);
        if(txnIt != txnLocks.end() && txnIt->second.count(key)) {
            // transaction already holds a lock on this key
            // if it's exclusive, we can grant any type
            // if it's shared and we want exclusive, we need to upgrade
            auto lockIt=lockTable.find(key);
            if(lockIt != lockTable.end()) {
                for(const auto& req : lockIt->second) {
                    if(req.txnId == txnId && req.granted) {
                        if(req.type == LockType::EXCLUSIVE || type == LockType::SHARED) {
                            // already have exclusive or requesting shared when have shared
                            return true;
                        }
                        // need to upgrade from shared to exclusive
                        // for now, just return true (simplified)
                        return true;
                    }
                }
            }
        }
        
        // check for deadlock first
        if(hasDeadlock(txnId)) {
            logError("deadlock detected for txn " + std::to_string(txnId));
            return false;
        }
        
        // add request to queue
        lockTable[key].emplace_back(txnId, type);
        txnWaits[txnId].insert(key);
        
        // wait until lock can be granted
        while(!canGrantLock(key, type)) {
            lockCondition.wait(lock);
            
            // check if we were aborted due to deadlock
            if(txnWaits[txnId].find(key) == txnWaits[txnId].end()) {
                return false; // aborted
            }
        }
        
        grantLock(key, txnId, type);
        lockCondition.notify_all();
        
        log("txn " + std::to_string(txnId) + " acquired " + 
            (type == LockType::SHARED ? "shared" : "exclusive") + " lock on " + key);
        
        return true;
    }
    
    bool LockManager::releaseLock(TransactionId txnId, const std::string& key)
    {
        std::lock_guard<std::mutex> lock(lockMutex);
        releaseLock(key, txnId);
        lockCondition.notify_all();
        
        log("txn " + std::to_string(txnId) + " released lock on " + key);
        return true;
    }
    
    bool LockManager::releaseAllLocks(TransactionId txnId)
    {
        std::lock_guard<std::mutex> lock(lockMutex);
        
        auto it=txnLocks.find(txnId);
        if(it != txnLocks.end()) {
            // copy the keys to avoid modifying set while iterating
            auto keys = it->second;
            for(const auto& key : keys) {
                releaseLock(key, txnId);
            }
            txnLocks.erase(it);
        }
        
        txnWaits.erase(txnId);
        lockCondition.notify_all();
        
        log("txn " + std::to_string(txnId) + " released all locks");
        return true;
    }
    
    bool LockManager::hasDeadlock(TransactionId txnId)
    {
       
        std::unordered_set<TransactionId> visited;
        std::unordered_set<TransactionId> recursionStack;
        
        std::function<bool(TransactionId)> hasCycle=[&](TransactionId current) -> bool {
            if(recursionStack.count(current)) return true; // cycle found
            if(visited.count(current)) return false;
            
            visited.insert(current);
            recursionStack.insert(current);
            
            // check what this txn is waiting for
            auto waitIt=txnWaits.find(current);
            if(waitIt != txnWaits.end()) {
                for(const auto& key : waitIt->second) {
                    // find who holds locks on this key
                    auto lockIt=lockTable.find(key);
                    if(lockIt != lockTable.end()) {
                        for(const auto& req : lockIt->second) {
                            if(req.granted && req.txnId != current) {
                                if(hasCycle(req.txnId)) return true;
                            }
                        }
                    }
                }
            }
            
            recursionStack.erase(current);
            return false;
        };
        
        return hasCycle(txnId);
    }
    
    void LockManager::printLockStatus()
    {
        std::lock_guard<std::mutex> lock(lockMutex);
        
        log("=== Lock Status ===");
        for(const auto& pair : lockTable) {
            const std::string& key=pair.first;
            const auto& requests=pair.second;
            
            std::string status="Key " + key + ": ";
            for(const auto& req : requests) {
                status += "T" + std::to_string(req.txnId) + 
                         (req.type == LockType::SHARED ? "S" : "X") +
                         (req.granted ? "+" : "-") + " ";
            }
            log(status);
        }
    }
}
