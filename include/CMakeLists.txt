file(GLOB_RECURSE CDB_LIB_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(cdblib OBJECT ${CDB_LIB_SRC_FILES})