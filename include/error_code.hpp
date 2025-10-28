#pragma once
#include<string>

namespace stonedb
{
    //Error code enum for detailed error reporting (GitHub Issue #7)
    enum class ErrorCode
    {
        SUCCESS=0,
        FILE_NOT_FOUND,
        FILE_OPEN_FAILED,
        FILE_READ_ERROR,
        FILE_WRITE_ERROR,
        FILE_SEEK_ERROR,
        INVALID_KEY,
        INVALID_VALUE,
        KEY_TOO_LARGE,
        VALUE_TOO_LARGE,
        RECORD_NOT_FOUND,
        PAGE_FULL,
        PAGE_NOT_FOUND,
        CACHE_FULL,
        TRANSACTION_NOT_FOUND,
        TRANSACTION_NOT_ACTIVE,
        LOCK_TIMEOUT,
        DEADLOCK_DETECTED,
        WAL_ERROR,
        STORAGE_ERROR,
        UNKNOWN_ERROR
    };
    
    std::string errorCodeToString(ErrorCode code);
}

