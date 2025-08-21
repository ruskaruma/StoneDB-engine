cmake_minimum_required(VERSION 3.12)
project(StonedDB-engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#idk, position independent whatever
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(stonedb STATIC src/PageManager.cpp) #it should be good enough

enable_testing()
add_executable(test_pagemanager tests/test_pagemanager.cpp)
target_link_libraries(test_pagemanager PRIVATE stonedb)
add_test(NAME PMTests COMMAND test_pagemanager)
