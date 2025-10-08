#include"common.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include"transaction.hpp"
#include<iostream>
#include<string>
#include<sstream>

void printHelp()
{
    std::cout << "StoneDB-engine v1.0.0 - Simple embedded database" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  put <key> <value>  - Store key-value pair" << std::endl;
    std::cout << "  get <key>          - Retrieve value for key" << std::endl;
    std::cout << "  del <key>          - Delete key" << std::endl;
    std::cout << "  scan               - Show all records" << std::endl;
    std::cout << "  help               - Show this help" << std::endl;
    std::cout << "  quit               - Exit database" << std::endl;
}

int main(int argc, char* argv[])
{
    stonedb::log("StoneDB-engine starting");
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    
    if(!storage->open("stonedb.sdb"))
    {
        std::cerr << "Failed to open database" << std::endl;
        return 1;
    }
    if(!wal->open("stonedb.wal"))
    {
        std::cerr << "Failed to open WAL" << std::endl;
        return 1;
    }
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    std::cout << "StoneDB-engine v1.0.0" << std::endl;
    std::cout << "Type 'help' for commands" << std::endl;
    
    std::string line;
    while(std::cout << "stonedb> " && std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        if(cmd == "quit" || cmd == "exit")
        {
            break;
        } else if(cmd == "help")
        {
            printHelp();
        } else if(cmd == "put") {
            std::string key, value;
            if(iss >> key >> value)
            {
                auto txnId=txnMgr.beginTransaction();
                if(txnMgr.putRecord(txnId, key, value))
                {
                    if(txnMgr.commitTransaction(txnId))
                    {
                        std::cout << "OK" << std::endl;
                    }
                    else
                    {
                        std::cout << "ERROR: commit failed" << std::endl;
                    }
                }
                else
                {
                    std::cout << "ERROR: put failed" << std::endl;
                    txnMgr.abortTransaction(txnId);
                }
            }
            else
            {
                std::cout << "Usage: put <key> <value>" << std::endl;
            }
        }
        else if(cmd == "get")
        {
            std::string key;
            if(iss >> key)
            {
                auto txnId=txnMgr.beginTransaction();
                std::string value;
                if(txnMgr.getRecord(txnId, key, value))
                {
                    std::cout << value << std::endl;
                }
                else
                {
                    std::cout << "NOT FOUND" << std::endl;
                }
                txnMgr.commitTransaction(txnId);
            }
            else
            {
                std::cout << "Usage: get <key>" << std::endl;
            }
        }
        else if(cmd == "del")
        {
            std::string key;
            if(iss >> key)
            {
                auto txnId=txnMgr.beginTransaction();
                if(txnMgr.deleteRecord(txnId, key))
                {
                    if(txnMgr.commitTransaction(txnId))
                    {
                        std::cout << "OK" << std::endl;
                    }
                    else
                    {
                        std::cout << "ERROR: commit failed" << std::endl;
                    }
                }
                else
                {
                    std::cout << "ERROR: delete failed" << std::endl;
                    txnMgr.abortTransaction(txnId);
                }
            }
            else
            {
                std::cout << "Usage: del <key>" << std::endl;
            }
        }
        else if(cmd == "scan")
        {
            auto records=storage->scanRecords();
            if(records.empty())
            {
                std::cout << "No records found" << std::endl;
            } else
            {
                for(const auto& record : records)
                {
                    std::cout << record.key << " = " << record.value << std::endl;
                }
            }
        }
        else if(cmd.empty())
        {

        }
        else
        {
            std::cout << "Unknown command: " << cmd << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }
    storage->close();
    wal->close();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
