#pragma once
#include<cstdint>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
#include<fstream>
#include<iostream>
namespace stonedb
{
    using TransactionId=uint64_t;
    using PageId=uint32_t;
    using SlotId=uint16_t;
    static constexpr size_t PAGE_SIZE=4096;
    static constexpr size_t MAX_KEY_SIZE=255;
    static constexpr size_t MAX_VALUE_SIZE=1024*1024;
    static constexpr TransactionId INVALID_TXN_ID = 0;
    struct Record
    {
        std::string key;
        std::string value;
        bool isDeleted=false;
        Record()=default;
        Record(const std::string& k, const std::string& v):key(k),value(v){}
    };
    struct Page
    {
        PageId pageId;
        std::vector<uint8_t> data;
        bool isDirty = false;    
        Page(PageId id):pageId(id),data(PAGE_SIZE, 0) {}
    };
    enum class LockType
    {
        SHARED,
        EXCLUSIVE
    };
    enum class LogType
    {
        BEGIN_TXN,
        COMMIT_TXN,
        ABORT_TXN,
        PUT_RECORD,
        DELETE_RECORD
    };
    struct LogEntry
    {
        LogType type;
        TransactionId txnId;
        std::string key;
        std::string value;
        uint64_t timestamp;    
        LogEntry(LogType t, TransactionId id) : type(t), txnId(id), timestamp(0) {}
    };
    void log(const std::string& msg);
    void logError(const std::string& msg);
}