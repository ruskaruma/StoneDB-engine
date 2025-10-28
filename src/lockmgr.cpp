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
        
        for(const auto& req : requests)
        {
            if(req.granted)
            {    
                if(requestedType == LockType::SHARED && req.type == LockType::SHARED)
                {
                    continue;
                }
                return false;
            }
        }
        return true;
    }

    void LockManager::grantLock(const std::string& key, TransactionId txnId, LockType type)
    {
        auto& requests=lockTable[key];
        for(auto& req : requests)
        {
            if(req.txnId == txnId && req.type == type && !req.granted)
            {
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
        requests.erase(std::remove_if(requests.begin(), requests.end(),
            [txnId](const LockRequest& req)
            {
                return req.txnId == txnId && req.granted;
            }), requests.end());
        if(requests.empty()) {
            lockTable.erase(it);
        }
        
        txnLocks[txnId].erase(key);
    }
    bool LockManager::acquireLock(TransactionId txnId, const std::string& key, LockType type)
    {
        std::unique_lock<std::mutex> lock(lockMutex);
        auto txnIt=txnLocks.find(txnId);
        if(txnIt != txnLocks.end() && txnIt->second.count(key))
        {
            auto lockIt=lockTable.find(key);
            if(lockIt != lockTable.end())
            {
                for(const auto& req : lockIt->second)
                {
                    if(req.txnId == txnId && req.granted)
                    {
                        //lock upgrade logic (bug #9)
                        if(req.type == LockType::EXCLUSIVE)
                        {
                            //already have exclusive, any request is granted
                            return true;
                        }
                        else if(req.type == LockType::SHARED && type == LockType::SHARED)
                        {
                            //already have shared, shared request granted
                            return true;
                        }
                        else if(req.type == LockType::SHARED && type == LockType::EXCLUSIVE)
                        {
                            //need to upgrade from shared to exclusive - not automatic
                            //will continue to request exclusive lock below
                            break;
                        }
                    }
                }
            }
        }
        

        if(hasDeadlock(txnId))
        {
            logError("deadlock detected for txn " + std::to_string(txnId));
            return false;
        }
        lockTable[key].emplace_back(txnId, type);
        txnWaits[txnId].insert(key);
        while(!canGrantLock(key, type))
        {
            lockCondition.wait(lock);
            if(txnWaits[txnId].find(key) == txnWaits[txnId].end())
            {
                return false;
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
        if(it != txnLocks.end())
        {
            auto keys = it->second;
            for(const auto& key : keys)
            {
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
        
        std::function<bool(TransactionId)> hasCycle=[&](TransactionId current) -> bool
        {
            if(recursionStack.count(current)) return true;
            if(visited.count(current)) return false;
            
            visited.insert(current);
            recursionStack.insert(current);
            auto waitIt=txnWaits.find(current);
            if(waitIt != txnWaits.end())
            {
                for(const auto& key : waitIt->second)
                {
                    auto lockIt=lockTable.find(key);
                    if(lockIt != lockTable.end())
                    {
                        for(const auto& req : lockIt->second)
                        {
                            if(req.granted && req.txnId != current)
                            {
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
        for(const auto& pair : lockTable)
        {
            const std::string& key=pair.first;
            const auto& requests=pair.second;
            std::string status="Key " + key + ": ";
            for(const auto& req : requests)
            {
                status += "T" + std::to_string(req.txnId) + 
                         (req.type == LockType::SHARED ? "S" : "X") +
                         (req.granted ? "+" : "-") + " ";
            }
            log(status);
        }
    }
}
