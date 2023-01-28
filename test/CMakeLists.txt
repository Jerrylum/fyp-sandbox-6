project(exchange_server_tests)

find_package(Catch2 3 REQUIRED)

# The test program
add_executable(tests testmain.cpp)
target_link_libraries(tests PRIVATE api Catch2::Catch2WithMain pthread)
