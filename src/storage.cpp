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
        
        if(!dbFile.is_open())
        {
            //creating new file
            dbFile.open(path, std::ios::out | std::ios::binary);
            if(!dbFile.is_open())
            {
                logError("failed to create db file: " + path);
                return false;
            }
            //writing header and initial page
            char header[HEADER_SIZE]={0};
            dbFile.write(header, HEADER_SIZE);
            
            // write initial page to ensure file is large enough
            char initialPage[PAGE_SIZE]={0};
            dbFile.write(initialPage, PAGE_SIZE);
            
            dbFile.flush();
            dbFile.close();
            //reopen for read/write
            dbFile.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if(!dbFile.is_open()) {
                logError("failed to reopen db file");
                return false;
            }
            log("reopened db file for read/write");
        }
        dbOpen=true;
        nextPageId=1;
        log("opened db: " + path);
        return true;
    }
    void StorageManager::close()
    {
        if(dbOpen)
        {
            flushAll();
            dbFile.close();
            pageCache.clear();
            dbOpen=false;
            log("closed db");
        }
    }
    bool StorageManager::writePageToDisk(PageId pageId, const std::vector<uint8_t>& data)
    {
        if(!dbOpen)
        {
            logError("database not open");
            return false;
        }
        
        dbFile.clear();
        
        std::streampos pos=HEADER_SIZE + (pageId * PAGE_SIZE);
        dbFile.seekp(pos);
        if(dbFile.fail())
        {
            logError("failed to seek to position " + std::to_string(pos));
            return false;
        }
        dbFile.write(reinterpret_cast<const char*>(data.data()), PAGE_SIZE);
        if(dbFile.fail())
        {
            logError("failed to write page data");
            return false;
        }
        dbFile.flush();
        if(dbFile.fail())
        {
            logError("failed to flush page data");
            return false;
        }
        return true;
    }
    bool StorageManager::readPageFromDisk(PageId pageId, std::vector<uint8_t>& data)
    {
        if(!dbOpen) return false;
        dbFile.clear();
        
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
        if(it != pageCache.end())
        {
            return it->second;
        }
        auto page=std::make_shared<Page>(pageId);
        if(!readPageFromDisk(pageId, page->data))
        {
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
                log("flushing page " + std::to_string(pair.first));
                if(!writePageToDisk(pair.first, pair.second->data))
                {
                    logError("failed to write page " + std::to_string(pair.first) + " to disk");
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
        
        // check if key already exists and update it
        auto keyPageIt=keyToPage.find(key);
        if(keyPageIt != keyToPage.end())
        {
            PageId existingPageId=keyPageIt->second;
            auto page=getPage(existingPageId);
            if(!page) return false;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                
                if(keyLen == 0) break;
                
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                {
                    // update in place if fits
                    if(valueLen >= value.size())
                    {
                        uint16_t newValueLen=value.size();
                        memcpy(page->data.data() + offset + 2, &newValueLen, 2);
                        memcpy(page->data.data() + offset + 4 + keyLen, value.c_str(), value.size());
                        page->isDirty=true;
                        return true;
                    }
                    else
                    {
                        // mark old record as deleted
                        uint16_t zero=0;
                        memcpy(page->data.data() + offset, &zero, 2);
                        page->isDirty=true;
                        keyToPage.erase(keyPageIt);
                        break;
                    }
                }
                offset += 4 + keyLen + valueLen;
            }
        }
        
        // try to insert in existing pages starting from page 0
        for(PageId pageId=0; pageId<nextPageId; pageId++)
        {
            auto page=getPage(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                
                if(keyLen == 0)
                {
                    // found free space
                    size_t needed=4 + key.size() + value.size();
                    if(offset + needed <= PAGE_SIZE)
                    {
                        keyLen=key.size();
                        valueLen=value.size();
                        memcpy(page->data.data() + offset, &keyLen, 2);
                        memcpy(page->data.data() + offset + 2, &valueLen, 2);
                        memcpy(page->data.data() + offset + 4, key.c_str(), keyLen);
                        memcpy(page->data.data() + offset + 4 + keyLen, value.c_str(), valueLen);
                        
                        page->isDirty=true;
                        keyToPage[key]=pageId;
                        return true;
                    }
                    break;
                }
                offset += 4 + keyLen + valueLen;
            }
        }
        
        // no space in existing pages, allocate new page
        PageId newPageId=allocateNewPage();
        auto newPage=getPage(newPageId);
        if(!newPage) return false;
        
        uint16_t keyLen=key.size();
        uint16_t valueLen=value.size();
        memcpy(newPage->data.data(), &keyLen, 2);
        memcpy(newPage->data.data() + 2, &valueLen, 2);
        memcpy(newPage->data.data() + 4, key.c_str(), keyLen);
        memcpy(newPage->data.data() + 4 + keyLen, value.c_str(), valueLen);
        
        newPage->isDirty=true;
        keyToPage[key]=newPageId;
        log("allocated new page " + std::to_string(newPageId) + " for key " + key);
        return true;
    }
    
    bool StorageManager::getRecord(const std::string& key, std::string& value)
    {
        // check keyToPage map first for faster lookup
        auto keyPageIt=keyToPage.find(key);
        if(keyPageIt != keyToPage.end())
        {
            PageId pageId=keyPageIt->second;
            auto page=getPage(pageId);
            if(!page) return false;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0) break;
                
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                {
                    value.assign(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                    return true;
                }
                offset += 4 + keyLen + valueLen;
            }
        }
        
        // fallback: search all pages
        for(PageId pageId=0; pageId<nextPageId; pageId++)
        {
            auto page=getPage(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0) break;
                
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                {
                    value.assign(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                    keyToPage[key]=pageId;
                    return true;
                }
                offset += 4 + keyLen + valueLen;
            }
        }
        return false;
    }
    bool StorageManager::deleteRecord(const std::string& key)
    {
        // check keyToPage map first
        auto keyPageIt=keyToPage.find(key);
        PageId targetPageId=0;
        
        if(keyPageIt != keyToPage.end())
        {
            targetPageId=keyPageIt->second;
        }
        
        // search for record
        for(PageId pageId=targetPageId; pageId<nextPageId; pageId++)
        {
            auto page=getPage(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0) break;
                
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                {
                    uint16_t zero=0;
                    memcpy(page->data.data() + offset, &zero, 2);
                    page->isDirty=true;
                    keyToPage.erase(key);
                    log("deleted record: " + key);
                    return true;
                }
                offset += 4 + keyLen + valueLen;
            }
            
            if(keyPageIt != keyToPage.end()) break;
        }
        
        logError("record not found for deletion: " + key);
        return false;
    }
    std::vector<Record> StorageManager::scanRecords()
    {
        std::vector<Record> records;
        
        // scan all pages
        for(PageId pageId=0; pageId<nextPageId; pageId++)
        {
            auto page=getPage(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0) break;
                
                std::string key(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                std::string value(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                records.emplace_back(key, value);
                offset += 4 + keyLen + valueLen;
            }
        }
        
        return records;
    }
    
    PageId StorageManager::allocateNewPage()
    {
        if(!freePages.empty())
        {
            PageId pageId=freePages.back();
            freePages.pop_back();
            return pageId;
        }
        return nextPageId++;
    }
    
    void StorageManager::deallocatePage(PageId pageId)
    {
        freePages.push_back(pageId);
    }
}
