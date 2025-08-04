add_library(stonedb STATIC
    PageManager.cpp
)
target_include_directories(stonedb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
