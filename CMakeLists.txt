cmake_minimum_required(VERSION 3.10)
project(cdb VERSION 0.0.0.1)

set(CMAKE_C_STANDARD 99)

add_compile_options(-Wall -g)

include_directories("${PROJECT_SOURCE_DIR}/debugger")

add_subdirectory("server")
add_subdirectory("debugger")

add_library(main OBJECT main.c)

add_executable(cdb $<TARGET_OBJECTS:main> $<TARGET_OBJECTS:server> $<TARGET_OBJECTS:debugger>)

target_link_libraries(cdb server ssl crypto cjson)
