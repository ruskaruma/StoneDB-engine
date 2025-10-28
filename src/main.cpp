#include"common.hpp"
#include"storage.hpp"
#include"wal.hpp"
#include"lockmgr.hpp"
#include"transaction.hpp"
#include"statistics.hpp"
#include<iostream>
#include<string>
#include<sstream>
#include<fstream>
#include<iomanip>
#include<cstdio>

void printHelp()
{
    std::cout << "StoneDB-engine v1.0.0 - Simple embedded database" << std::endl;
    std::cout << "Usage: stone [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -d, --db PATH      Database file path (default: stonedb.sdb)" << std::endl;
    std::cout << "  -b, --batch        Batch mode (non-interactive)" << std::endl;
    std::cout << "  -q, --quiet       Suppress log messages" << std::endl;
    std::cout << "  -h, --help         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  put <key> <value>  - Store key-value pair" << std::endl;
    std::cout << "  get <key>          - Retrieve value for key" << std::endl;
    std::cout << "  del <key>          - Delete key" << std::endl;
    std::cout << "  scan               - Show all records" << std::endl;
    std::cout << "  backup <path>      - Backup database to JSON file" << std::endl;
    std::cout << "  restore <path>     - Restore database from JSON file" << std::endl;
    std::cout << "  stats              - Show database statistics" << std::endl;
    std::cout << "  help               - Show this help" << std::endl;
    std::cout << "  quit               - Exit database" << std::endl;
}

int main(int argc, char* argv[])
{
    std::string dbPath="stonedb.sdb";
    bool batchMode=false;
    bool quietMode=false;
    
    //parse command line arguments
    for(int i=1; i<argc; i++)
    {
        std::string arg=argv[i];
        if(arg == "--help" || arg == "-h")
        {
            printHelp();
            return 0;
        }
        else if(arg == "--batch" || arg == "-b")
        {
            batchMode=true;
        }
        else if(arg == "--quiet" || arg == "-q")
        {
            quietMode=true;
        }
        else if(arg == "--db" || arg == "-d")
        {
            if(i+1 < argc)
            {
                dbPath=argv[++i];
            }
            else
            {
                std::cerr << "Error: --db requires a path argument" << std::endl;
                return 1;
            }
        }
        else if(arg[0] == '-')
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return 1;
        }
    }
    
    //derive WAL path from database path
    std::string walPath=dbPath;
    size_t lastDot=walPath.find_last_of('.');
    if(lastDot != std::string::npos)
    {
        walPath=walPath.substr(0, lastDot) + ".wal";
    }
    else
    {
        walPath += ".wal";
    }
    
    if(!quietMode)
    {
        stonedb::log("StoneDB-engine starting");
    }
    
    auto storage=std::make_shared<stonedb::StorageManager>();
    auto wal=std::make_shared<stonedb::WALManager>();
    auto lockMgr=std::make_shared<stonedb::LockManager>();
    stonedb::Statistics stats;
    
    if(!storage->open(dbPath))
    {
        std::cerr << "Failed to open database: " << dbPath << std::endl;
        return 1;
    }
    if(!wal->open(walPath))
    {
        std::cerr << "Failed to open WAL: " << walPath << std::endl;
        return 1;
    }
    stonedb::TransactionManager txnMgr(storage, wal, lockMgr);
    
    if(!batchMode)
    {
        std::cout << "StoneDB-engine v1.0.0" << std::endl;
        std::cout << "Database: " << dbPath << std::endl;
        std::cout << "Type 'help' for commands" << std::endl;
    }
    
    std::string line;
    while(true)
    {
        if(batchMode)
        {
            if(!std::getline(std::cin, line))
            {
                break;
            }
        }
        else
        {
            std::cout << "stonedb> ";
            if(!std::getline(std::cin, line))
            {
                break;
            }
        }
        
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        if(cmd == "quit" || cmd == "exit")
        {
            break;
        }
        else if(cmd.empty())
        {
            continue;
        }
        else if(cmd == "help")
        {
            printHelp();
        }
        else if(cmd == "stats")
        {
            //GitHub Issue #9: statistics command
            std::cout << "=== Database Statistics ===" << std::endl;
            std::cout << "Transactions: " << stats.getTransactionCount() << std::endl;
            std::cout << "PUT operations: " << stats.getPutOps() << std::endl;
            std::cout << "GET operations: " << stats.getGetOps() << std::endl;
            std::cout << "DELETE operations: " << stats.getDeleteOps() << std::endl;
            std::cout << "Cache hits: " << stats.getCacheHits() << std::endl;
            std::cout << "Cache misses: " << stats.getCacheMisses() << std::endl;
            std::cout << "Cache hit ratio: " << std::fixed << std::setprecision(2) << stats.getCacheHitRatio() << "%" << std::endl;
            std::cout << "Lock waits: " << stats.getLockWaits() << std::endl;
            std::cout << "Deadlocks detected: " << stats.getDeadlocks() << std::endl;
        } else if(cmd == "put") {
            //fix bug #14: handle multi-word values with proper parsing
            std::string key;
            if(iss >> key)
            {
                //get rest of line as value (handles spaces and quotes)
                std::string value;
                std::getline(iss, value);
                
                //trim leading whitespace
                if(!value.empty() && value[0] == ' ')
                {
                    value.erase(0, 1);
                }
                
                //handle quoted values
                if(!value.empty() && value.front() == '"' && value.back() == '"' && value.size() >= 2)
                {
                    value=value.substr(1, value.size() - 2);
                }
                
                //validate input (bug #19 partial fix)
                if(key.empty())
                {
                    std::cout << "ERROR: Key cannot be empty" << std::endl;
                    continue;
                }
                auto txnId=txnMgr.beginTransaction();
                stats.incrementTransactions();
                stats.incrementPutOps();
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
                std::cout << "       put <key> \"value with spaces\"" << std::endl;
            }
        }
        else if(cmd == "get")
        {
            std::string key;
            if(iss >> key)
            {
                auto txnId=txnMgr.beginTransaction();
                stats.incrementGetOps();
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
                stats.incrementDeleteOps();
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
            //fix bug #15: add transaction isolation for scan
            auto txnId=txnMgr.beginTransaction();
            auto records=storage->scanRecords();
            txnMgr.commitTransaction(txnId);
            
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
        else if(cmd == "backup")
        {
            std::string backupPath;
            if(iss >> backupPath)
            {
                auto records=storage->scanRecords();
                std::ofstream outFile(backupPath);
                if(!outFile.is_open())
                {
                    std::cout << "ERROR: Cannot open backup file: " << backupPath << std::endl;
                    continue;
                }
                
                outFile << "{\n";
                outFile << "  \"stonedb_backup\": true,\n";
                outFile << "  \"version\": \"1.0.0\",\n";
                outFile << "  \"records\": [\n";
                
                for(size_t i=0; i<records.size(); i++)
                {
                    outFile << "    {\n";
                    outFile << "      \"key\": \"" << records[i].key << "\",\n";
                    //fix bug #12: escape all JSON special characters
                    std::string escapedValue=records[i].value;
                    size_t pos=0;
                    
                    //escape backslashes first
                    while((pos=escapedValue.find("\\", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\\\");
                        pos += 2;
                    }
                    pos=0;
                    
                    //escape quotes
                    while((pos=escapedValue.find("\"", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\\"");
                        pos += 2;
                    }
                    pos=0;
                    
                    //escape control characters
                    while((pos=escapedValue.find("\b", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\b");
                        pos += 2;
                    }
                    pos=0;
                    while((pos=escapedValue.find("\f", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\f");
                        pos += 2;
                    }
                    pos=0;
                    while((pos=escapedValue.find("\n", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\n");
                        pos += 2;
                    }
                    pos=0;
                    while((pos=escapedValue.find("\r", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\r");
                        pos += 2;
                    }
                    pos=0;
                    while((pos=escapedValue.find("\t", pos)) != std::string::npos)
                    {
                        escapedValue.replace(pos, 1, "\\t");
                        pos += 2;
                    }
                    
                    //escape other control characters (0x00-0x1F)
                    pos=0;
                    while(pos < escapedValue.size())
                    {
                        unsigned char c=escapedValue[pos];
                        if(c < 0x20 && c != '\b' && c != '\t' && c != '\n' && c != '\f' && c != '\r')
                        {
                            char hex[5];
                            snprintf(hex, sizeof(hex), "\\u%04x", c);
                            escapedValue.replace(pos, 1, hex);
                            pos += 6;
                        }
                        else
                        {
                            pos++;
                        }
                    }
                    outFile << "      \"value\": \"" << escapedValue << "\"\n";
                    outFile << "    }";
                    if(i < records.size()-1)
                    {
                        outFile << ",";
                    }
                    outFile << "\n";
                }
                
                outFile << "  ]\n";
                outFile << "}\n";
                outFile.close();
                std::cout << "Backup saved to " << backupPath << " (" << records.size() << " records)" << std::endl;
            }
            else
            {
                std::cout << "Usage: backup <path>" << std::endl;
            }
        }
        else if(cmd == "restore")
        {
            std::string backupPath;
            if(iss >> backupPath)
            {
                std::ifstream inFile(backupPath);
                if(!inFile.is_open())
                {
                    std::cout << "ERROR: Cannot open backup file: " << backupPath << std::endl;
                    continue;
                }
                
                std::string line;
                std::string json;
                while(std::getline(inFile, line))
                {
                    json += line + "\n";
                }
                inFile.close();
                
                size_t recordsStart=json.find("\"records\":");
                if(recordsStart == std::string::npos)
                {
                    std::cout << "ERROR: Invalid backup format" << std::endl;
                    continue;
                }
                
                size_t pos=recordsStart;
                int restoreCount=0;
                auto txnId=txnMgr.beginTransaction();
                
                while((pos=json.find("\"key\":", pos)) != std::string::npos)
                {
                    pos += 7;
                    while(pos < json.size() && (json[pos] == ' ' || json[pos] == '\"'))
                        pos++;
                    size_t keyStart=pos;
                    while(pos < json.size() && json[pos] != '\"')
                        pos++;
                    std::string key=json.substr(keyStart, pos-keyStart);
                    
                    pos=json.find("\"value\":", pos);
                    if(pos == std::string::npos) break;
                    pos += 8;
                    while(pos < json.size() && (json[pos] == ' ' || json[pos] == '\"'))
                        pos++;
                    size_t valueStart=pos;
                    std::string value;
                    
                    //parse value with proper escape handling
                    while(pos < json.size())
                    {
                        if(json[pos] == '\\' && pos+1 < json.size())
                        {
                            if(json[pos+1] == '\"')
                            {
                                value += '"';
                                pos += 2;
                            }
                            else if(json[pos+1] == '\\')
                            {
                                value += '\\';
                                pos += 2;
                            }
                            else if(json[pos+1] == 'n')
                            {
                                value += '\n';
                                pos += 2;
                            }
                            else if(json[pos+1] == 't')
                            {
                                value += '\t';
                                pos += 2;
                            }
                            else if(json[pos+1] == 'r')
                            {
                                value += '\r';
                                pos += 2;
                            }
                            else
                            {
                                value += json[pos];
                                pos++;
                            }
                        }
                        else if(json[pos] == '"')
                        {
                            pos++;
                            //skip whitespace after closing quote
                            while(pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                                pos++;
                            if(pos < json.size() && (json[pos] == ',' || json[pos] == '}' || json[pos] == ']'))
                            {
                                break;
                            }
                            value += '"';
                        }
                        else if(json[pos] == ',' || json[pos] == '}' || json[pos] == ']')
                        {
                            break;
                        }
                        else
                        {
                            value += json[pos];
                            pos++;
                        }
                    }
                    
                    if(txnMgr.putRecord(txnId, key, value))
                    {
                        restoreCount++;
                    }
                }
                
                if(txnMgr.commitTransaction(txnId))
                {
                    std::cout << "Restore completed: " << restoreCount << " records restored" << std::endl;
                }
                else
                {
                    std::cout << "ERROR: Restore failed during commit" << std::endl;
                    txnMgr.abortTransaction(txnId);
                }
            }
            else
            {
                std::cout << "Usage: restore <path>" << std::endl;
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
    
    if(!batchMode)
    {
        std::cout << "Goodbye!" << std::endl;
    }
    return 0;
}
