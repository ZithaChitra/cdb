include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dwarf")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/handlers")

file(GLOB_RECURSE DEBUGGER_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(debugger OBJECT ${DEBUGGER_SRC_FILES})
