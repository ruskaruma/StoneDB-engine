#include "wal.hpp"
#include "storage.hpp"
#include<chrono>
#include<cstring>

namespace stonedb
{
    WALManager::WALManager() : walOpen(false) {}
    WALManager::~WALManager()
    {
        if(walOpen)
        {
            flush();
            close();
        }
    }
    bool WALManager::open(const std::string& path)
    {
        walPath=path;
        walFile.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
        if(!walFile.is_open())
        {
            walFile.open(path, std::ios::out | std::ios::binary);
            if(!walFile.is_open())
            {
                logError("failed to create wal file: " + path);
                return false;
            }
            char header[WAL_HEADER_SIZE] = {0};
            walFile.write(header, WAL_HEADER_SIZE);
            walFile.flush();
            walFile.close();
            
            //fix bug #16: explicitly ensure closed before retry
            if(walFile.is_open())
            {
                walFile.close();
            }
            
            walFile.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
            if(!walFile.is_open())
            {
                logError("failed to reopen wal file");
                return false;
            }
        }
        walOpen = true;
        log("opened wal: " + path);
        return true;
    }
    void WALManager::close()
    {
        if(walOpen) 
        {
            flush();
            walFile.close();
            committedTxns.clear();
            activeTxns.clear();
            walOpen = false;
            log("closed wal");
        }
    }
    void WALManager::serializeEntry(const LogEntry& entry, std::vector<uint8_t>& data)
    {
        size_t keySize = entry.key.size();
        size_t valueSize = entry.value.size();
        size_t totalSize = sizeof(LogType) + sizeof(TransactionId) + sizeof(uint64_t) + sizeof(uint16_t) + keySize + sizeof(uint16_t) + valueSize;
        data.resize(totalSize);
        size_t offset = 0;
        memcpy(data.data() + offset, &entry.type, sizeof(LogType));
        offset+=sizeof(LogType);
        memcpy(data.data() + offset, &entry.txnId, sizeof(TransactionId));
        offset+=sizeof(TransactionId);
        memcpy(data.data() + offset, &entry.timestamp, sizeof(uint64_t));
        offset+=sizeof(uint64_t);
        uint16_t keyLen = keySize;
        memcpy(data.data() + offset, &keyLen, sizeof(uint16_t));
        offset+=sizeof(uint16_t);
        memcpy(data.data() + offset, entry.key.c_str(), keySize);
        offset+=keySize;
        uint16_t valueLen=valueSize;
        memcpy(data.data()+ offset, &valueLen, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(data.data()+offset, entry.value.c_str(), valueSize);
    }
    bool WALManager::deserializeEntry(const std::vector<uint8_t>& data, LogEntry& entry)
    {
        size_t offset = 0;
        if(offset + sizeof(LogType) > data.size()) return false;
        memcpy(&entry.type, data.data() + offset, sizeof(LogType));
        offset += sizeof(LogType);
        if(offset +sizeof(TransactionId) > data.size()) return false;
        memcpy(&entry.txnId, data.data() + offset, sizeof(TransactionId));
        offset += sizeof(TransactionId);

        if(offset + sizeof(uint64_t) > data.size()) return false;
        memcpy(&entry.timestamp, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        if(offset + sizeof(uint16_t) > data.size()) return false;
        uint16_t keyLen;
        memcpy(&keyLen, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if(offset + keyLen > data.size()) return false;
        entry.key.assign(reinterpret_cast<const char*>(data.data() + offset), keyLen);
        offset+=keyLen;
        if(offset + sizeof(uint16_t) > data.size()) return false;
        uint16_t valueLen;
        memcpy(&valueLen, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        if(offset + valueLen > data.size()) return false;
        entry.value.assign(reinterpret_cast<const char*>(data.data() + offset), valueLen);

        return true;
    }
    bool WALManager::writeLogEntry(const LogEntry& entry)
    {
        if(!walOpen) return false;
        walFile.clear();
        std::vector<uint8_t> data;
        serializeEntry(entry, data);
        walFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        if(walFile.fail()) return false;
        return true;
    }
    bool WALManager::logBeginTxn(TransactionId txnId)
    {
        LogEntry entry(LogType::BEGIN_TXN, txnId);
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if(!writeLogEntry(entry)) return false;
        activeTxns.insert(txnId);
        return true;
    }
    bool WALManager::logCommitTxn(TransactionId txnId)
    {
        LogEntry entry(LogType::COMMIT_TXN, txnId);
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if(!writeLogEntry(entry)) return false;
        activeTxns.erase(txnId);
        committedTxns.insert(txnId);
        return flush();
    }
    bool WALManager::logAbortTxn(TransactionId txnId)
    {
        LogEntry entry(LogType::ABORT_TXN, txnId);
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if(!writeLogEntry(entry)) return false;
        activeTxns.erase(txnId);
        return true;
    }
    bool WALManager::logPutRecord(TransactionId txnId, const std::string& key, const std::string& value)
    {
        LogEntry entry(LogType::PUT_RECORD, txnId);
        entry.key = key;
        entry.value = value;
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return writeLogEntry(entry);
    }
    bool WALManager::logDeleteRecord(TransactionId txnId, const std::string& key)
    {
        LogEntry entry(LogType::DELETE_RECORD, txnId);
        entry.key = key;
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return writeLogEntry(entry);
    }
    bool WALManager::flush()
    {
        if(!walOpen)
        {
            logError("WAL not open");
            return false;
        }
        walFile.clear();
        walFile.flush();
        if(walFile.fail())
        {
            logError("WAL flush failed");
            return false;
        }
        walFile.sync();
        if(walFile.fail())
        {
            logError("WAL sync failed");
            return false;
        }
        return true;
    }
    std::vector<LogEntry> WALManager::replayLog()
    {
        std::vector<LogEntry> entries;
        if(!walOpen) return entries;
        walFile.seekg(WAL_HEADER_SIZE);
        if(walFile.fail()) return entries;
        while(walFile.good())
        {
            char header[sizeof(LogType) + sizeof(TransactionId) + sizeof(uint64_t) + 2*sizeof(uint16_t)];
            walFile.read(header, sizeof(header));
            if(walFile.gcount()!=sizeof(header)) break;

            uint16_t keyLen, valueLen;
            memcpy(&keyLen, header + sizeof(LogType) + sizeof(TransactionId) + sizeof(uint64_t), sizeof(uint16_t));
            memcpy(&valueLen, header + sizeof(LogType) + sizeof(TransactionId) + sizeof(uint64_t) + sizeof(uint16_t), sizeof(uint16_t));

            //fix bug #11: validate sizes before allocating to prevent DoS
            if(keyLen > MAX_KEY_SIZE || valueLen > MAX_VALUE_SIZE)
            {
                logError("invalid keyLen or valueLen in WAL entry - possible corruption");
                break;
            }
            
            //validate reasonable limits to prevent huge allocations
            const size_t MAX_WAL_KEY_SIZE=1024*1024;
            const size_t MAX_WAL_VALUE_SIZE=10*1024*1024;
            if(keyLen > MAX_WAL_KEY_SIZE || valueLen > MAX_WAL_VALUE_SIZE)
            {
                logError("WAL entry size exceeds maximum - possible corruption");
                break;
            }

            std::vector<char> keyData(keyLen);
            std::vector<char> valueData(valueLen);
            walFile.read(keyData.data(), keyLen);
            walFile.read(valueData.data(), valueLen);
            if(walFile.gcount() != keyLen + valueLen) break;

            LogEntry entry(LogType::BEGIN_TXN, 0);
            memcpy(&entry.type, header, sizeof(LogType));
            memcpy(&entry.txnId, header + sizeof(LogType), sizeof(TransactionId));
            memcpy(&entry.timestamp, header + sizeof(LogType) + sizeof(TransactionId), sizeof(uint64_t));
            entry.key.assign(keyData.data(), keyLen);
            entry.value.assign(valueData.data(), valueLen);

            entries.push_back(entry);
        }

        std::vector<LogEntry> committedEntries;
        std::unordered_set<TransactionId> committed;

        for(const auto& entry : entries)
        {
            if(entry.type == LogType::COMMIT_TXN) committed.insert(entry.txnId);
        }

        for(const auto& entry : entries)
        {
            if(committed.count(entry.txnId) && (entry.type == LogType::PUT_RECORD || entry.type == LogType::DELETE_RECORD))
            {
                committedEntries.push_back(entry);
            }
        }
        log("replayed " + std::to_string(committedEntries.size()) + " committed operations");
        return committedEntries;
    }
    
    bool WALManager::checkpoint(std::shared_ptr<class StorageManager> storage)
    {
        if(!walOpen) return false;
        
        //flush all storage pages
        if(storage)
        {
            storage->flushAll();
        }
        
        //flush WAL
        if(!flush())
        {
            return false;
        }
        
        log("checkpoint completed");
        return true;
    }
    
    bool WALManager::truncateLog()
    {
        //fix bug #20: truncate WAL after checkpoint
        if(!walOpen) return false;
        
        walFile.flush();
        walFile.close();
        
        //truncate file to header size (remove all log entries)
        walFile.open(walPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if(!walFile.is_open())
        {
            logError("failed to truncate WAL file");
            return false;
        }
        
        char header[WAL_HEADER_SIZE]={0};
        walFile.write(header, WAL_HEADER_SIZE);
        walFile.flush();
        walFile.close();
        
        //reopen in append mode
        walFile.open(walPath, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
        if(!walFile.is_open())
        {
            logError("failed to reopen WAL after truncation");
            return false;
        }
        
        committedTxns.clear();
        activeTxns.clear();
        log("WAL truncated");
        return true;
    }
}
