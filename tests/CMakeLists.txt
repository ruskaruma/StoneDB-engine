add_executable(test_pagemanager test_pagemanager.cpp)
target_link_libraries(test_pagemanager stonedb)

add_test(NAME PageManagerTest COMMAND test_pagemanager)
message(STATUS "Adding test PageManagerTest")
