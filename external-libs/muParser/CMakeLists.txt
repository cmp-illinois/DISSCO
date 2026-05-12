cmake_minimum_required(VERSION 3.25)

project(MUPARSER)
message(STATUS "== MUPARSER ==")

file(GLOB_RECURSE MUPARSER_SRC
    "${PROJECT_SOURCE_DIR}/*.cpp"
    "${PROJECT_SOURCE_DIR}/*.h"
)

### in the future, we'll let this be shared if there's a viable muparser lib install
add_library(MUPARSER STATIC ${MUPARSER_SRC})

target_include_directories(MUPARSER PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(MUPARSER PUBLIC MUPARSER_STATIC)

message(STATUS "DONE!")