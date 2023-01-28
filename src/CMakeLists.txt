project(exchange_server)

# All sources that also need to be tested in unit tests go into a static library
add_library(api STATIC api.cpp)
# set_target_properties(api PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
target_include_directories(api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The main program
add_executable(server main.cpp)
# set_target_properties(server PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
target_link_libraries(server PRIVATE api)

