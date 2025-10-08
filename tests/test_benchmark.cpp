#include"transaction.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<iostream>
#include<chrono>
#include<random>

int main()
{
    std::cout << "StoneDB Benchmark Test" << std::endl;
    
    // create components
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    // initialize database
    storage->open("benchmark.sdb");
    wal->open("benchmark.wal");
    
    // create transaction manager
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    const int numOperations=1000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    // benchmark write operations
    std::cout << "Benchmarking " << numOperations << " write operations..." << std::endl;
    auto start=std::chrono::high_resolution_clock::now();
    
    for(int i=0; i<numOperations; ++i) {
        auto txnId=txnMgr.beginTransaction();
        std::string key="key" + std::to_string(i);
        std::string value="value" + std::to_string(i);
        txnMgr.putRecord(txnId, key, value);
        txnMgr.commitTransaction(txnId);
    }
    
    auto end=std::chrono::high_resolution_clock::now();
    auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Write operations: " << duration.count() << "ms" << std::endl;
    std::cout << "Write throughput: " << (numOperations * 1000.0 / duration.count()) << " ops/sec" << std::endl;
    
    // benchmark read operations
    std::cout << "Benchmarking " << numOperations << " read operations..." << std::endl;
    start=std::chrono::high_resolution_clock::now();
    
    for(int i=0; i<numOperations; ++i) {
        auto txnId=txnMgr.beginTransaction();
        std::string key="key" + std::to_string(i);
        std::string value;
        txnMgr.getRecord(txnId, key, value);
        txnMgr.commitTransaction(txnId);
    }
    
    end=std::chrono::high_resolution_clock::now();
    duration=std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Read operations: " << duration.count() << "ms" << std::endl;
    std::cout << "Read throughput: " << (numOperations * 1000.0 / duration.count()) << " ops/sec" << std::endl;
    
    // benchmark mixed operations
    std::cout << "Benchmarking " << numOperations << " mixed operations..." << std::endl;
    start=std::chrono::high_resolution_clock::now();
    
    for(int i=0; i<numOperations; ++i) {
        auto txnId=txnMgr.beginTransaction();
        
        if(i % 3 == 0) {
            // write operation
            std::string key="mixed" + std::to_string(i);
            std::string value="mixed_value" + std::to_string(i);
            txnMgr.putRecord(txnId, key, value);
        } else {
            // read operation
            std::string key="key" + std::to_string(dis(gen));
            std::string value;
            txnMgr.getRecord(txnId, key, value);
        }
        
        txnMgr.commitTransaction(txnId);
    }
    
    end=std::chrono::high_resolution_clock::now();
    duration=std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Mixed operations: " << duration.count() << "ms" << std::endl;
    std::cout << "Mixed throughput: " << (numOperations * 1000.0 / duration.count()) << " ops/sec" << std::endl;
    
    // cleanup
    storage->close();
    wal->close();
    
    // remove test files
    std::remove("benchmark.sdb");
    std::remove("benchmark.wal");
    
    std::cout << "Benchmark test completed" << std::endl;
    return 0;
}
