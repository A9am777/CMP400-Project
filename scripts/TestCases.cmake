add_executable(TestApp ${CMAKE_CURRENT_LIST_DIR}/../src/Testing/Tests.cpp)
target_link_libraries(TestApp Catch2::Catch2WithMain)