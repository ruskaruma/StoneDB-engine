cmake_minimum_required(VERSION 3.16)
project(StoneDB-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include directories
include_directories(include)

# source files
set(SOURCES
    src/common.cpp
    src/storage.cpp
    src/wal.cpp
    src/lockmgr.cpp
    src/transaction.cpp
)

# create library
add_library(stonedb ${SOURCES})

# create executable
add_executable(stone src/main.cpp)
target_link_libraries(stone stonedb)

# test executables
add_executable(test_common tests/test_common.cpp)
target_link_libraries(test_common stonedb)

add_executable(test_storage tests/test_storage.cpp)
target_link_libraries(test_storage stonedb)

add_executable(test_wal tests/test_wal.cpp)
target_link_libraries(test_wal stonedb)

add_executable(test_lockmgr tests/test_lockmgr.cpp)
target_link_libraries(test_lockmgr stonedb)

add_executable(test_transaction tests/test_transaction.cpp)
target_link_libraries(test_transaction stonedb)

add_executable(test_recovery tests/test_recovery.cpp)
target_link_libraries(test_recovery stonedb)

add_executable(test_benchmark tests/test_benchmark.cpp)
target_link_libraries(test_benchmark stonedb)

add_executable(test_concurrent tests/test_concurrent.cpp)
target_link_libraries(test_concurrent stonedb)

add_executable(test_integration tests/test_integration.cpp)
target_link_libraries(test_integration stonedb)

add_executable(test_multipage tests/test_multipage.cpp)
target_link_libraries(test_multipage stonedb)

# compiler flags
target_compile_options(stonedb PRIVATE -Wall -Wextra -O2)
target_compile_options(stone PRIVATE -Wall -Wextra -O2)
target_compile_options(test_common PRIVATE -Wall -Wextra -O2)
target_compile_options(test_storage PRIVATE -Wall -Wextra -O2)
target_compile_options(test_wal PRIVATE -Wall -Wextra -O2)
target_compile_options(test_lockmgr PRIVATE -Wall -Wextra -O2)
target_compile_options(test_transaction PRIVATE -Wall -Wextra -O2)
