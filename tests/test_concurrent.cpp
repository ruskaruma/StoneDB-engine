#include"transaction.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include<iostream>
#include<thread>
#include<vector>
#include<chrono>
#include<random>

int main()
{
    std::cout << "Testing concurrent transactions..." << std::endl;
    
    // create components
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    // initialize database
    storage->open("concurrent_test.sdb");
    wal->open("concurrent_test.wal");
    
    // create transaction manager
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    const int numThreads=4;
    const int operationsPerThread=50;
    std::vector<std::thread> threads;
    std::vector<bool> results(numThreads, false);
    
    // test concurrent writes
    std::cout << "Testing concurrent writes..." << std::endl;
    for(int i=0; i<numThreads; ++i) {
        threads.emplace_back([&, i]() {
            bool success=true;
            for(int j=0; j<operationsPerThread; ++j) {
                auto txnId=txnMgr.beginTransaction();
                std::string key="thread" + std::to_string(i) + "_key" + std::to_string(j);
                std::string value="thread" + std::to_string(i) + "_value" + std::to_string(j);
                
                if(!txnMgr.putRecord(txnId, key, value)) {
                    success=false;
                    txnMgr.abortTransaction(txnId);
                    break;
                }
                
                if(!txnMgr.commitTransaction(txnId)) {
                    success=false;
                    break;
                }
            }
            results[i]=success;
        });
    }
    
    // wait for all threads
    for(auto& thread : threads) {
        thread.join();
    }
    
    // check results
    bool allSuccess=true;
    for(int i=0; i<numThreads; ++i) {
        if(!results[i]) {
            std::cout << "Thread " << i << " failed" << std::endl;
            allSuccess=false;
        }
    }
    
    if(allSuccess) {
        std::cout << "All concurrent write threads succeeded" << std::endl;
    } else {
        std::cout << "Some concurrent write threads failed" << std::endl;
        return 1;
    }
    
    // test concurrent reads
    std::cout << "Testing concurrent reads..." << std::endl;
    threads.clear();
    results.assign(numThreads, false);
    
    for(int i=0; i<numThreads; ++i) {
        threads.emplace_back([&, i]() {
            bool success=true;
            for(int j=0; j<operationsPerThread; ++j) {
                auto txnId=txnMgr.beginTransaction();
                std::string key="thread" + std::to_string(i) + "_key" + std::to_string(j);
                std::string value;
                
                if(!txnMgr.getRecord(txnId, key, value)) {
                    success=false;
                    txnMgr.abortTransaction(txnId);
                    break;
                }
                
                if(!txnMgr.commitTransaction(txnId)) {
                    success=false;
                    break;
                }
            }
            results[i]=success;
        });
    }
    
    // wait for all threads
    for(auto& thread : threads) {
        thread.join();
    }
    
    // check results
    allSuccess=true;
    for(int i=0; i<numThreads; ++i) {
        if(!results[i]) {
            std::cout << "Thread " << i << " failed" << std::endl;
            allSuccess=false;
        }
    }
    
    if(allSuccess) {
        std::cout << "All concurrent read threads succeeded" << std::endl;
    } else {
        std::cout << "Some concurrent read threads failed" << std::endl;
        return 1;
    }
    
    // test mixed operations
    std::cout << "Testing mixed concurrent operations..." << std::endl;
    threads.clear();
    results.assign(numThreads, false);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    for(int i=0; i<numThreads; ++i) {
        threads.emplace_back([&, i]() {
            bool success=true;
            for(int j=0; j<operationsPerThread; ++j) {
                auto txnId=txnMgr.beginTransaction();
                std::string key="mixed_thread" + std::to_string(i) + "_key" + std::to_string(j);
                
                int operation=dis(gen);
                if(operation==0) {
                    // write operation
                    std::string value="mixed_thread" + std::to_string(i) + "_value" + std::to_string(j);
                    if(!txnMgr.putRecord(txnId, key, value)) {
                        success=false;
                        txnMgr.abortTransaction(txnId);
                        break;
                    }
                } else if(operation==1) {
                    // read operation
                    std::string value;
                    if(!txnMgr.getRecord(txnId, key, value)) {
                        // key might not exist, that's ok
                    }
                } else {
                    // delete operation
                    if(!txnMgr.deleteRecord(txnId, key)) {
                        // key might not exist, that's ok
                    }
                }
                
                if(!txnMgr.commitTransaction(txnId)) {
                    success=false;
                    break;
                }
            }
            results[i]=success;
        });
    }
    
    // wait for all threads
    for(auto& thread : threads) {
        thread.join();
    }
    
    // check results
    allSuccess=true;
    for(int i=0; i<numThreads; ++i) {
        if(!results[i]) {
            std::cout << "Thread " << i << " failed" << std::endl;
            allSuccess=false;
        }
    }
    
    if(allSuccess) {
        std::cout << "All mixed concurrent threads succeeded" << std::endl;
    } else {
        std::cout << "Some mixed concurrent threads failed" << std::endl;
        return 1;
    }
    
    // cleanup
    storage->close();
    wal->close();
    
    // remove test files
    std::remove("concurrent_test.sdb");
    std::remove("concurrent_test.wal");
    
    std::cout << "Concurrent transaction tests passed" << std::endl;
    return 0;
}
