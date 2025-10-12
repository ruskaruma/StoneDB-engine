#include"storage.hpp"
#include<cassert>

int main()
{
    // clean up old test file
    std::remove("test_db.sdb");
    
    stonedb::StorageManager storage;
    //testing basic operations
    assert(storage.open("test_db.sdb"));
    assert(storage.isOpen());
    //testing put/get
    assert(storage.putRecord("key1", "value1"));
    assert(storage.putRecord("key2", "value2"));   
    std::string value;
    assert(storage.getRecord("key1", value));
    assert(value == "value1");   
    assert(storage.getRecord("key2", value));
    assert(value == "value2");
    //testing scan
    auto records=storage.scanRecords();
    assert(records.size() == 2);  
    //testing delete
    assert(storage.deleteRecord("key1"));
    assert(!storage.getRecord("key1", value));
    storage.close();
    stonedb::log("storage tests passed");
    return 0;
}
