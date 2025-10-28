#include"error_code.hpp"

namespace stonedb
{
    std::string errorCodeToString(ErrorCode code)
    {
        switch(code)
        {
            case ErrorCode::SUCCESS: return "Success";
            case ErrorCode::FILE_NOT_FOUND: return "File not found";
            case ErrorCode::FILE_OPEN_FAILED: return "File open failed";
            case ErrorCode::FILE_READ_ERROR: return "File read error";
            case ErrorCode::FILE_WRITE_ERROR: return "File write error";
            case ErrorCode::FILE_SEEK_ERROR: return "File seek error";
            case ErrorCode::INVALID_KEY: return "Invalid key";
            case ErrorCode::INVALID_VALUE: return "Invalid value";
            case ErrorCode::KEY_TOO_LARGE: return "Key too large";
            case ErrorCode::VALUE_TOO_LARGE: return "Value too large";
            case ErrorCode::RECORD_NOT_FOUND: return "Record not found";
            case ErrorCode::PAGE_FULL: return "Page full";
            case ErrorCode::PAGE_NOT_FOUND: return "Page not found";
            case ErrorCode::CACHE_FULL: return "Cache full";
            case ErrorCode::TRANSACTION_NOT_FOUND: return "Transaction not found";
            case ErrorCode::TRANSACTION_NOT_ACTIVE: return "Transaction not active";
            case ErrorCode::LOCK_TIMEOUT: return "Lock timeout";
            case ErrorCode::DEADLOCK_DETECTED: return "Deadlock detected";
            case ErrorCode::WAL_ERROR: return "WAL error";
            case ErrorCode::STORAGE_ERROR: return "Storage error";
            case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
            default: return "Unknown error code";
        }
    }
}

