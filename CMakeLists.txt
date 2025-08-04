cmake_minimum_required(VERSION 3.10)
project(StoneDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()                      # <= MUST be here

add_subdirectory(src)
add_subdirectory(tests)
