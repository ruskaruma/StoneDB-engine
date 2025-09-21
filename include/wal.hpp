#pragma once
#include"common.hpp"
#include<fstream>
#include<vector>
#include<unordered_set>

namespace stonedb
{
    class WALManager
    {
    private:
        std::string walPath;
        std::fstream walFile;
        bool walOpen;
        std::unordered_set<TransactionId> committedTxns;
        std::unordered_set<TransactionId> activeTxns;
        
        static constexpr size_t WAL_HEADER_SIZE=32;
        
        bool writeLogEntry(const LogEntry& entry);
        bool readLogEntry(LogEntry& entry);
        void serializeEntry(const LogEntry& entry, std::vector<uint8_t>& data);
        bool deserializeEntry(const std::vector<uint8_t>& data, LogEntry& entry);
        
    public:
        WALManager();
        ~WALManager();
        
        bool open(const std::string& path);
        void close();
        bool isOpen() const
        {
            return walOpen;
        }
        
       //LOGGINGS

        bool logBeginTxn(TransactionId txnId);
        bool logCommitTxn(TransactionId txnId);
        bool logAbortTxn(TransactionId txnId);
        bool logPutRecord(TransactionId txnId, const std::string& key, const std::string& value);
        bool logDeleteRecord(TransactionId txnId, const std::string& key);
        std::vector<LogEntry> replayLog();
        bool flush();
    };
}
