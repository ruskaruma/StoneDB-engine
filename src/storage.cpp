#include"storage.hpp"
#include<filesystem>
#include<cstring>
namespace stonedb
{
    StorageManager::StorageManager() : nextPageId(1), dbOpen(false)
    {
    } 
    StorageManager::~StorageManager()
    {
        if(dbOpen)
        {
            flushAll();
            close();
        }
    }
    bool StorageManager::open(const std::string& path)
    {
        dbPath=path;
        dbFile.open(path, std::ios::in | std::ios::out | std::ios::binary);
        
        if(!dbFile.is_open()) {
            //creating new file
            dbFile.open(path, std::ios::out | std::ios::binary);
            if(!dbFile.is_open()) {
                logError("failed to create db file: " + path);
                return false;
            }
            
            //writing header
            char header[HEADER_SIZE]={0};
            dbFile.write(header, HEADER_SIZE);
            dbFile.flush();
            dbFile.close();
            
            //reopen for read/write
            dbFile.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if(!dbFile.is_open()) {
                logError("failed to reopen db file");
                return false;
            }
        }
        dbOpen=true;
        nextPageId=1;
        log("opened db: " + path);
        return true;
    }
    void StorageManager::close()
    {
        if(dbOpen) {
            flushAll();
            dbFile.close();
            pageCache.clear();
            dbOpen=false;
            log("closed db");
        }
    }
    bool StorageManager::writePageToDisk(PageId pageId, const std::vector<uint8_t>& data)
    {
        if(!dbOpen) return false; 
        std::streampos pos=HEADER_SIZE + (pageId * PAGE_SIZE);
        dbFile.seekp(pos);
        if(dbFile.fail()) return false;
        dbFile.write(reinterpret_cast<const char*>(data.data()), PAGE_SIZE);
        if(dbFile.fail()) return false;
        dbFile.flush();
        return true;
    }
    bool StorageManager::readPageFromDisk(PageId pageId, std::vector<uint8_t>& data)
    {
        if(!dbOpen) return false;
        std::streampos pos=HEADER_SIZE + (pageId * PAGE_SIZE);
        dbFile.seekg(pos);
        if(dbFile.fail()) return false;
        
        data.resize(PAGE_SIZE);
        dbFile.read(reinterpret_cast<char*>(data.data()), PAGE_SIZE);
        if(dbFile.fail()) return false;
        
        return true;
    }
    
    std::shared_ptr<Page> StorageManager::getPage(PageId pageId)
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        auto it=pageCache.find(pageId);
        if(it != pageCache.end()) {
            return it->second;
        }
        
        //loading from disk
        auto page=std::make_shared<Page>(pageId);
        if(!readPageFromDisk(pageId, page->data)) {
            //creating new page
            page->data.assign(PAGE_SIZE, 0);
        }
        
        pageCache[pageId]=page;
        return page;
    }
    
    bool StorageManager::flushPage(PageId pageId)
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        auto it=pageCache.find(pageId);
        if(it == pageCache.end()) return true;
        
        if(it->second->isDirty)
        {
            if(!writePageToDisk(pageId, it->second->data))
            {
                return false;
            }
            it->second->isDirty=false;
        }
        return true;
    }

    bool StorageManager::flushAll()
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        for(auto& pair : pageCache)
        {
            if(pair.second->isDirty)
            {
                if(!writePageToDisk(pair.first, pair.second->data))
                {
                    return false;
                }
                pair.second->isDirty=false;
            }
        }
        return true;
    }
    
    bool StorageManager::putRecord(const std::string& key, const std::string& value)
    {
        if(key.size() > MAX_KEY_SIZE || value.size() > MAX_VALUE_SIZE)
        {
            return false;
        }
        
        //simplified implementation, putting everything in page 0      
        auto page=getPage(0);
        if(!page) return false;
        
        //finding free space in page
        size_t offset=0;
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE) {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            
            if(keyLen == 0)
            {          
                keyLen=key.size();
                valueLen=value.size();
                memcpy(page->data.data() + offset, &keyLen, 2);
                memcpy(page->data.data() + offset + 2, &valueLen, 2);
                memcpy(page->data.data() + offset + 4, key.c_str(), keyLen);
                memcpy(page->data.data() + offset + 4 + keyLen, value.c_str(), valueLen);
                
                page->isDirty=true;
                return true;
            }
            
            offset += 4 + keyLen + valueLen;
        }
        
        logError("page full, need better allocation");
        return false;
    }
    
    bool StorageManager::getRecord(const std::string& key, std::string& value)
    {
        auto page=getPage(0);
        if(!page) return false;
        
        size_t offset=0;
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
        {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            if(keyLen==0) break;
            std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
            if(recordKey==key)
            {
                value.assign(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                return true;
            }
            offset+=4+keyLen+valueLen;
        }
        return false;
    }
    bool StorageManager::deleteRecord(const std::string& key)
    {
        auto page=getPage(0);
        if(!page) return false;   
        size_t offset=0;
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
        {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            if(keyLen==0) break;
            std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
            if(recordKey==key)
            {
                uint16_t zero=0;
                memcpy(page->data.data() + offset, &zero, 2);
                page->isDirty=true;
                return true;
            }
            offset += 4 + keyLen + valueLen;
        }
        return false;
    }
    std::vector<Record> StorageManager::scanRecords()
    {
        std::vector<Record> records;
        auto page=getPage(0);
        if(!page) return records;   
        size_t offset=0;
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
        {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            if(keyLen==0) break;
            std::string key(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
            std::string value(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
            records.emplace_back(key, value);
            offset += 4 + keyLen + valueLen;
        }
        return records;
    }
}
