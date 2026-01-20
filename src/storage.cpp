#include"storage.hpp"
#include<filesystem>
#include<cstring>
#include<algorithm>
#include<limits>
namespace stonedb
{
    StorageManager::StorageManager() : nextPageId(1), dbOpen(false)
    {
        allocatedPages.insert(0);
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
            
            //write initial page to ensure file is large enough
            char initialPage[PAGE_SIZE]={0};
            dbFile.write(initialPage, PAGE_SIZE);
            
            dbFile.flush();
            dbFile.close();
            
            //explicitly close before retry to avoid resource leak
            if(dbFile.is_open())
            {
                dbFile.close();
            }
            dbFile.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if(!dbFile.is_open()) {
                logError("failed to reopen db file");
                return false;
            }
            log("reopened db file for read/write");
        }
        dbOpen=true;
        allocatedPages.clear();
        allocatedPages.insert(0);
        keyToPage.clear();
        freePages.clear();
        dbFile.seekg(0, std::ios::end);
        std::streampos fileSize=dbFile.tellg();
        std::streamoff fileSizeBytes=static_cast<std::streamoff>(fileSize);
        if(fileSizeBytes > static_cast<std::streamoff>(HEADER_SIZE))
        {
            PageId maxPageId=static_cast<PageId>((fileSizeBytes - static_cast<std::streamoff>(HEADER_SIZE)) / static_cast<std::streamoff>(PAGE_SIZE));
            nextPageId=maxPageId + 1;
            for(PageId pageId=0; pageId<=maxPageId; pageId++)
            {
                auto page=getPage(pageId);
                if(page)
                {
                    allocatedPages.insert(pageId);
                    size_t offset=0;
                    while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
                    {
                        uint16_t keyLen, valueLen;
                        memcpy(&keyLen, page->data.data() + offset, 2);
                        memcpy(&valueLen, page->data.data() + offset + 2, 2);
                        if(keyLen == 0)
                        {
                            if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                            {
                                offset += 4 + valueLen;
                                continue;
                            }
                            size_t searchOffset=offset + 4;
                            bool foundNext=false;
                            while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                            {
                                if(searchOffset + 4 > PAGE_SIZE) break;
                                uint16_t testKeyLen, testValueLen;
                                memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                                memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                                
                                if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                                   testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                                   searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                                {
                                    offset=searchOffset;
                                    foundNext=true;
                                    break;
                                }
                                if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE)
                                {
                                    searchOffset += 4 + testValueLen;
                                    continue;
                                }
                                searchOffset += 4;
                                if(searchOffset > offset + PAGE_SIZE / 2) break;
                            }
                            if(!foundNext) break;
                            continue;
                        }
                        if(offset + 4 > PAGE_SIZE || 
                           offset + 4 + keyLen > PAGE_SIZE ||
                           offset + 4 + keyLen + valueLen > PAGE_SIZE ||
                           keyLen > MAX_KEY_SIZE || valueLen > MAX_VALUE_SIZE)
                        {
                            break;
                        }
                        
                        std::string key(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                        keyToPage[key]=pageId;
                        
                        offset += 4 + keyLen + valueLen;
                    }
                }
            }
        }
        else
        {
        nextPageId=1;
        }
        
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
        if(!dbOpen) {
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
        if(it != pageCache.end()) {
            return it->second;
        }
        auto page=std::make_shared<Page>(pageId);
        if(!readPageFromDisk(pageId, page->data))
        {
            page->data.assign(PAGE_SIZE, 0);
        }
        while(pageCache.size() >= MAX_CACHE_SIZE)
        {
            evictLRUPage();
        }
        
        pageCache[pageId]=page;
        return page;
    }
    
    std::shared_ptr<Page> StorageManager::getPageUnlocked(PageId pageId)
    {
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
        
        size_t requiredSize=4 + key.size() + value.size();
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it=keyToPage.find(key);
        if(it != keyToPage.end())
        {
            PageId pageId=it->second;
            if(findFreeSpaceInPage(pageId, key, value, requiredSize))
            {
                keyToPage[key]=pageId;
                return true;
            }
            keyToPage.erase(key);
        }
        for(PageId pageId : allocatedPages)
        {
            if(findFreeSpaceInPage(pageId, key, value, requiredSize))
            {
                keyToPage[key]=pageId;
                return true;
            }
        }
        PageId newPageId=allocateNewPage();
        if(findFreeSpaceInPage(newPageId, key, value, requiredSize))
        {
            keyToPage[key]=newPageId;
            return true;
        }
        logError("failed to allocate space for record");
        return false;
    }
    
    bool StorageManager::getRecord(const std::string& key, std::string& value)
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it=keyToPage.find(key);
        if(it != keyToPage.end())
        {
            PageId pageId=it->second;
            auto page=getPageUnlocked(pageId);
            if(page)
            {
        size_t offset=0;
                while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
                {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
                    if(keyLen == 0)
                    {
                        if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                        {
                            offset += 4 + valueLen;
                            continue;
                        }
                        size_t searchOffset=offset + 4;
                        bool foundNext=false;
                        while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                        {
                            if(searchOffset + 4 > PAGE_SIZE) break;
                            uint16_t testKeyLen, testValueLen;
                            memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                            memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                            
                            if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                               testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                               searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                            {
                                offset=searchOffset;
                                foundNext=true;
                                break;
                            }
                            if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE &&
                               searchOffset + 4 + testValueLen < PAGE_SIZE)
                            {
                                searchOffset += 4 + testValueLen;
                                continue;
                            }
                            searchOffset += 4;
                        }
                        if(!foundNext) break;
                        continue;
                    }
                    if(offset + 4 > PAGE_SIZE || 
                       offset + 4 + keyLen > PAGE_SIZE ||
                       offset + 4 + keyLen + valueLen > PAGE_SIZE)
                    {
                        break;
                    }
            std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                    if(recordKey == key)
                    {
                value.assign(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                return true;
                    }
                    
                    offset += 4 + keyLen + valueLen;
                }
            }
        }
        for(PageId pageId : allocatedPages)
        {
            auto page=getPageUnlocked(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0)
                {
                    if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                    {
                        offset += 4 + valueLen;
                        continue;
                    }
                    size_t searchOffset=offset + 4;
                    bool foundNext=false;
                    while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                    {
                        if(searchOffset + 4 > PAGE_SIZE) break;
                        uint16_t testKeyLen, testValueLen;
                        memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                        memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                        
                        if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                           testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                           searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                        {
                            offset=searchOffset;
                            foundNext=true;
                            break;
                        }
                        if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE &&
                           searchOffset + 4 + testValueLen < PAGE_SIZE)
                        {
                            searchOffset += 4 + testValueLen;
                            continue;
                        }
                        searchOffset += 4;
                    }
                    if(!foundNext) break;
                    continue;
                }
                if(offset + 4 > PAGE_SIZE || 
                   offset + 4 + keyLen > PAGE_SIZE ||
                   offset + 4 + keyLen + valueLen > PAGE_SIZE)
                {
                    break;
                }
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                    {
                        value.assign(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
                        //update mapping for future lookups
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
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it=keyToPage.find(key);
        if(it != keyToPage.end())
        {
            PageId pageId=it->second;
            auto page=getPageUnlocked(pageId);
            if(page)
            {
                size_t offset=0;
                while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
                {
                    uint16_t keyLen, valueLen;
                    memcpy(&keyLen, page->data.data() + offset, 2);
                    memcpy(&valueLen, page->data.data() + offset + 2, 2);
                    
                    //skip deleted records
                    if(keyLen == 0)
                    {
                        if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                        {
                            offset += 4 + valueLen;
                            continue;
                        }
                        size_t searchOffset=offset + 4;
                        bool foundNext=false;
                        while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                        {
                            if(searchOffset + 4 > PAGE_SIZE) break;
                            uint16_t testKeyLen, testValueLen;
                            memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                            memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                            
                            if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                               testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                               searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                            {
                                offset=searchOffset;
                                foundNext=true;
                                break;
                            }
                            if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE &&
                               testValueLen < MAX_VALUE_SIZE && searchOffset + 4 + testValueLen < PAGE_SIZE)
                            {
                                searchOffset += 4 + testValueLen;
                                continue;
                            }
                            searchOffset += 4;
                        }
                        if(!foundNext) break;
                        continue;
                    }
                    if(offset + 4 > PAGE_SIZE || 
                       offset + 4 + keyLen > PAGE_SIZE ||
                       offset + 4 + keyLen + valueLen > PAGE_SIZE)
                    {
                        break;
                    }
                    
                    std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                    if(recordKey == key)
                    {
                        uint16_t zero=0;
                        memcpy(page->data.data() + offset, &zero, 2);
                        page->isDirty=true;
                        keyToPage.erase(key);
                        return true;
                    }
                    
                    offset += 4 + keyLen + valueLen;
                }
            }
        }
        for(PageId pageId : allocatedPages)
        {
            auto page=getPageUnlocked(pageId);
            if(!page) continue;
            
            size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
                uint16_t keyLen, valueLen;
                memcpy(&keyLen, page->data.data() + offset, 2);
                memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0)
                {
                    if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                    {
                        offset += 4 + valueLen;
                        continue;
                    }
                    size_t searchOffset=offset + 4;
                    bool foundNext=false;
                    while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                    {
                        if(searchOffset + 4 > PAGE_SIZE) break;
                        uint16_t testKeyLen, testValueLen;
                        memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                        memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                        
                        if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                           testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                           searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                        {
                            offset=searchOffset;
                            foundNext=true;
                            break;
                        }
                        if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE &&
                           searchOffset + 4 + testValueLen < PAGE_SIZE)
                        {
                            searchOffset += 4 + testValueLen;
                            continue;
                        }
                        searchOffset += 4;
                    }
                    if(!foundNext) break;
                    continue;
                }
                if(offset + 4 > PAGE_SIZE || 
                   offset + 4 + keyLen > PAGE_SIZE ||
                   offset + 4 + keyLen + valueLen > PAGE_SIZE)
                {
                    break;
                }
                std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
                if(recordKey == key)
                {
                    uint16_t zero=0;
                    memcpy(page->data.data() + offset, &zero, 2);
                    page->isDirty=true;
                    keyToPage.erase(key);
                    return true;
                }
                
                offset += 4 + keyLen + valueLen;
            }
        }
        
        return false;
    }
    PageId StorageManager::allocateNewPage()
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        if(!freePages.empty())
        {
            PageId pageId=freePages.back();
            freePages.pop_back();
            allocatedPages.insert(pageId);
            return pageId;
        }
        PageId newPageId=nextPageId++;
        const size_t maxPageId=std::numeric_limits<std::streampos>::max() / PAGE_SIZE;
        if(newPageId > maxPageId)
        {
            logError("page ID overflow: maximum pages exceeded");
            return 0;
        }
        
        allocatedPages.insert(newPageId);
        size_t pageSizeBytes=static_cast<size_t>(newPageId) * PAGE_SIZE;
        std::streamoff maxFileSize=std::numeric_limits<std::streamoff>::max();
        std::streamoff totalSize=static_cast<std::streamoff>(HEADER_SIZE) + static_cast<std::streamoff>(pageSizeBytes) + static_cast<std::streamoff>(PAGE_SIZE);
        if(static_cast<std::streamoff>(pageSizeBytes) > maxFileSize - static_cast<std::streamoff>(HEADER_SIZE) - static_cast<std::streamoff>(PAGE_SIZE))
        {
            logError("file size calculation overflow");
            allocatedPages.erase(newPageId);
            return 0;
        }
        std::streampos requiredSize=totalSize;
        dbFile.seekp(0, std::ios::end);
        std::streampos currentSize=dbFile.tellp();
        if(currentSize < requiredSize)
        {
            std::streampos targetPos=static_cast<std::streampos>(totalSize - 1);
            dbFile.seekp(targetPos);
            dbFile.write("\0", 1);
            dbFile.flush();
        }
        return newPageId;
    }
    void StorageManager::deallocatePage(PageId pageId)
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        allocatedPages.erase(pageId);
        freePages.push_back(pageId);
        pageCache.erase(pageId);
    }
    
    void StorageManager::evictLRUPage()
    {
        for(auto it=pageCache.begin(); it != pageCache.end(); ++it)
        {
            if(!it->second->isDirty)
            {
                pageCache.erase(it);
                return;
            }
        }
        if(!pageCache.empty())
        {
            pageCache.erase(pageCache.begin());
        }
    }
    
    bool StorageManager::findFreeSpaceInPage(PageId pageId, const std::string& key, const std::string& value, size_t requiredSize)
    {
        auto page=getPageUnlocked(pageId);
        if(!page) return false;   
        
        //check if record exists in this page
        size_t offset=0;
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
        {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            if(keyLen == 0)
            {
                if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                {
                    offset += 4 + valueLen;
                    continue;
                }
                break;
            }
            if(offset + 4 > PAGE_SIZE || 
               offset + 4 + keyLen > PAGE_SIZE ||
               offset + 4 + keyLen + valueLen > PAGE_SIZE)
            {
                break;
            }
            
            std::string recordKey(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
            if(recordKey == key)
            {
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
                uint16_t zero=0;
                memcpy(page->data.data() + offset, &zero, 2);
                    page->isDirty=true;
                    break;
                }
            }
            
            offset += 4 + keyLen + valueLen;
        }
        offset=0;
        
        while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
        {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
            
            if(keyLen == 0)
            {
                size_t slotSize=valueLen > 0 && valueLen < MAX_VALUE_SIZE ? valueLen : 0;
                size_t slotTotalSize=4 + slotSize;
                if(slotTotalSize >= requiredSize)
                {
                    //reuse this deleted slot
                    keyLen=key.size();
                    valueLen=value.size();
                    memcpy(page->data.data() + offset, &keyLen, 2);
                    memcpy(page->data.data() + offset + 2, &valueLen, 2);
                    memcpy(page->data.data() + offset + 4, key.c_str(), keyLen);
                    memcpy(page->data.data() + offset + 4 + keyLen, value.c_str(), valueLen);
                page->isDirty=true;
                return true;
                }
                if(slotSize > 0 && slotSize < MAX_VALUE_SIZE && offset + 4 + slotSize < PAGE_SIZE)
                {
                    size_t newOffset=offset + 4 + slotSize;
                    if(newOffset <= offset) break; //prevent infinite loop
                    offset=newOffset;
                    continue;
                }
                else
                {
                    size_t searchOffset=offset + 4;
                    bool foundNext=false;
                    while(searchOffset < PAGE_SIZE - 4)
                    {
                        uint16_t testKeyLen;
                        memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                        if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE)
                        {
                            offset=searchOffset;
                            foundNext=true;
                            break;
                        }
                        searchOffset += 4;
                        if(searchOffset > offset + 100) break;
                    }
                    if(!foundNext) break;
                    continue;
                }
            }
            
            //validate bounds
            if(offset + 4 > PAGE_SIZE || 
               offset + 4 + keyLen > PAGE_SIZE ||
               offset + 4 + keyLen + valueLen > PAGE_SIZE)
            {
                break;
            }
            
            offset += 4 + keyLen + valueLen;
        }
        if(offset < PAGE_SIZE - RECORD_HEADER_SIZE && offset + requiredSize <= PAGE_SIZE)
        {
            if(offset + 4 <= PAGE_SIZE)
            {
                uint16_t nextKeyLen, nextValueLen;
                memcpy(&nextKeyLen, page->data.data() + offset, 2);
                memcpy(&nextValueLen, page->data.data() + offset + 2, 2);
                if(nextKeyLen == 0 && nextValueLen == 0)
                {
                    uint16_t keyLen=key.size();
                    uint16_t valueLen=value.size();
                    memcpy(page->data.data() + offset, &keyLen, 2);
                    memcpy(page->data.data() + offset + 2, &valueLen, 2);
                    memcpy(page->data.data() + offset + 4, key.c_str(), keyLen);
                    memcpy(page->data.data() + offset + 4 + keyLen, value.c_str(), valueLen);
                    page->isDirty=true;
                    return true;
                }
            }
        }
        
        return false;
    }
    std::vector<Record> StorageManager::scanRecords()
    {
        std::vector<Record> records;
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        for(PageId pageId : allocatedPages)
        {
            auto page=getPageUnlocked(pageId);
            if(!page) continue;
            
        size_t offset=0;
            while(offset < PAGE_SIZE - RECORD_HEADER_SIZE)
            {
            uint16_t keyLen, valueLen;
            memcpy(&keyLen, page->data.data() + offset, 2);
            memcpy(&valueLen, page->data.data() + offset + 2, 2);
                if(keyLen == 0)
                {
                    if(valueLen > 0 && valueLen < MAX_VALUE_SIZE && offset + 4 + valueLen < PAGE_SIZE)
                    {
                        offset += 4 + valueLen;
                        continue;
                    }
                    size_t searchOffset=offset + 4;
                    bool foundNext=false;
                    while(searchOffset < PAGE_SIZE - RECORD_HEADER_SIZE)
                    {
                        if(searchOffset + 4 > PAGE_SIZE) break;
                        
                        uint16_t testKeyLen, testValueLen;
                        memcpy(&testKeyLen, page->data.data() + searchOffset, 2);
                        memcpy(&testValueLen, page->data.data() + searchOffset + 2, 2);
                        if(testKeyLen > 0 && testKeyLen <= MAX_KEY_SIZE && 
                           testValueLen > 0 && testValueLen <= MAX_VALUE_SIZE &&
                           searchOffset + 4 + testKeyLen + testValueLen <= PAGE_SIZE)
                        {
                            offset=searchOffset;
                            foundNext=true;
                            break;
                        }
                        if(testKeyLen == 0 && testValueLen > 0 && testValueLen < MAX_VALUE_SIZE && 
                           searchOffset + 4 + testValueLen < PAGE_SIZE)
                        {
                            searchOffset += 4 + testValueLen;
                            continue;
                        }
                        searchOffset += 4;
                    }
                    if(!foundNext) break;
                    continue;
                }
                if(keyLen > MAX_KEY_SIZE || valueLen > MAX_VALUE_SIZE)
                {
                    break;
                }
                if(offset + 4 > PAGE_SIZE || 
                   offset + 4 + keyLen > PAGE_SIZE ||
                   offset + 4 + keyLen + valueLen > PAGE_SIZE)
                {
                    break;
                }
            std::string key(reinterpret_cast<const char*>(page->data.data() + offset + 4), keyLen);
            std::string value(reinterpret_cast<const char*>(page->data.data() + offset + 4 + keyLen), valueLen);
            records.emplace_back(key, value);
                
            offset += 4 + keyLen + valueLen;
            }
        }
        
        return records;
    }
}
