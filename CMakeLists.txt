cmake_minimum_required(VERSION 3.10)
project(cdb VERSION 0.0.0.1)

set(CMAKE_C_STANDARD 99)

add_compile_options(-Wall -g)

set(SOURCE_FILES bfviwer.c io.c server.c)

# Add the executable target
add_executable(server ${SOURCE_FILES})

# Link against the required libraries
target_link_libraries(server ssl crypto cjson)