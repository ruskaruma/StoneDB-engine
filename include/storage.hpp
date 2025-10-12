#pragma once
#include"common.hpp"
#include<fstream>
#include<unordered_map>
#include<mutex>

namespace stonedb
{
    class StorageManager
    {
    private:
        std::string dbPath;
        std::fstream dbFile;
        std::unordered_map<PageId, std::shared_ptr<Page>> pageCache;
        std::mutex cacheMutex;
        PageId nextPageId;
        bool dbOpen;
        std::unordered_map<std::string, PageId> keyToPage;
        std::vector<PageId> freePages;
        
        static constexpr size_t HEADER_SIZE=64;
        static constexpr size_t RECORD_HEADER_SIZE=8;
        
        bool writePageToDisk(PageId pageId, const std::vector<uint8_t>& data);
        bool readPageFromDisk(PageId pageId, std::vector<uint8_t>& data);
        PageId allocateNewPage();
        void deallocatePage(PageId pageId);
        
    public:
        StorageManager();
        ~StorageManager();
        
        bool open(const std::string& path);
        void close();
        bool isOpen() const { return dbOpen; }
        
        std::shared_ptr<Page> getPage(PageId pageId);
        bool flushPage(PageId pageId);
        bool flushAll();
        
        bool putRecord(const std::string& key, const std::string& value);
        bool getRecord(const std::string& key, std::string& value);
        bool deleteRecord(const std::string& key);
        
        // simple scan for now
        std::vector<Record> scanRecords();
    };
}
