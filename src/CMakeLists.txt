project(exchange_server)

# All sources that also need to be tested in unit tests go into a static library
add_library(api STATIC api.cpp)
target_include_directories(api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The main program
add_executable(server main.cpp)
target_link_libraries(server PRIVATE api)

