cmake_minimum_required(VERSION 3.16)
project(StoneDB-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include directories
include_directories(include)

# source files
set(SOURCES
    src/common.cpp
)

# create library
add_library(stonedb ${SOURCES})

# create executable
add_executable(stone src/main.cpp)
target_link_libraries(stone stonedb)

# test executable
add_executable(test_stonedb tests/test_common.cpp)
target_link_libraries(test_stonedb stonedb)

# compiler flags
target_compile_options(stonedb PRIVATE -Wall -Wextra -O2)
target_compile_options(stone PRIVATE -Wall -Wextra -O2)
target_compile_options(test_stonedb PRIVATE -Wall -Wextra -O2)
