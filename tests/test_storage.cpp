#include"storage.hpp"
#include<cassert>

int main()
{
    stonedb::StorageManager storage;
    
    //test basic operations
    assert(storage.open("test_db.sdb"));
    assert(storage.isOpen());
    
    //test put/get
    assert(storage.putRecord("key1", "value1"));
    assert(storage.putRecord("key2", "value2"));
    
    std::string value;
    assert(storage.getRecord("key1", value));
    assert(value == "value1");
    
    assert(storage.getRecord("key2", value));
    assert(value == "value2");
    
    //test scan
    auto records=storage.scanRecords();
    assert(records.size() == 2);
    
    //test delete
    assert(storage.deleteRecord("key1"));
    assert(!storage.getRecord("key1", value));
    
    storage.close();
    stonedb::log("storage tests passed");
    return 0;
}
