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

# compiler flags
target_compile_options(stonedb PRIVATE -Wall -Wextra -O2)
target_compile_options(stone PRIVATE -Wall -Wextra -O2)
target_compile_options(test_common PRIVATE -Wall -Wextra -O2)
target_compile_options(test_storage PRIVATE -Wall -Wextra -O2)
