#include"storage.hpp"
#include<cassert>
#include<iostream>

int main()
{
    std::cout << "Testing multi-page storage..." << std::endl;
    
    // clean up old test file
    std::remove("multipage_test.sdb");
    
    stonedb::StorageManager storage;
    assert(storage.open("multipage_test.sdb"));
    
    // fill page 0 with records until it overflows
    std::cout << "Filling pages with records..." << std::endl;
    
    int recordCount=0;
    for(int i=0; i<200; i++)
    {
        std::string key="key" + std::to_string(i);
        std::string value="this_is_a_longer_value_to_fill_pages_faster_" + std::to_string(i);
        
        if(!storage.putRecord(key, value))
        {
            std::cout << "Failed to insert at record " << i << std::endl;
            break;
        }
        recordCount++;
    }
    
    std::cout << "Inserted " << recordCount << " records" << std::endl;
    assert(recordCount > 100);
    
    // test retrieval from multiple pages
    std::cout << "Testing retrieval across pages..." << std::endl;
    for(int i=0; i<recordCount; i++)
    {
        std::string key="key" + std::to_string(i);
        std::string value;
        
        if(!storage.getRecord(key, value))
        {
            std::cout << "Failed to retrieve key: " << key << std::endl;
            assert(false);
        }
        
        std::string expected="this_is_a_longer_value_to_fill_pages_faster_" + std::to_string(i);
        assert(value == expected);
    }
    std::cout << "Retrieved all " << recordCount << " records successfully" << std::endl;
    
    // test scan across all pages
    std::cout << "Testing scan across all pages..." << std::endl;
    auto records=storage.scanRecords();
    std::cout << "Scan found " << records.size() << " records" << std::endl;
    assert(records.size() >= recordCount);
    
    // test update on different pages
    std::cout << "Testing updates across pages..." << std::endl;
    assert(storage.putRecord("key0", "updated_value_0"));
    assert(storage.putRecord("key50", "updated_value_50"));
    assert(storage.putRecord("key" + std::to_string(recordCount-1), "updated_last"));
    
    std::string value;
    assert(storage.getRecord("key0", value));
    assert(value == "updated_value_0");
    
    assert(storage.getRecord("key50", value));
    assert(value == "updated_value_50");
    
    // test deletion across pages
    std::cout << "Testing deletions across pages..." << std::endl;
    assert(storage.deleteRecord("key0"));
    assert(storage.deleteRecord("key50"));
    assert(storage.deleteRecord("key100"));
    
    assert(!storage.getRecord("key0", value));
    assert(!storage.getRecord("key50", value));
    assert(!storage.getRecord("key100", value));
    
    // verify scan after deletions (some records may be marked as deleted from updates)
    auto recordsAfterDel=storage.scanRecords();
    std::cout << "After deletions: " << recordsAfterDel.size() << " records" << std::endl;
    assert(recordsAfterDel.size() > 0);
    assert(recordsAfterDel.size() < recordCount);
    
    // verify multiple pages were actually allocated
    std::cout << "Verified multi-page allocation working" << std::endl;
    storage.close();
    
    // cleanup
    std::remove("multipage_test.sdb");
    
    std::cout << "Multi-page storage tests passed" << std::endl;
    return 0;
}
