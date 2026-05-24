cmake_minimum_required(VERSION 3.25)

project(PUGIXML)
message(STATUS "== PUGIXML ==")

### Vendored pugixml 1.15 (https://pugixml.org/, MIT licensed). ###
### Three source files; no further configuration needed. ###

add_library(PUGIXML STATIC
    ${PROJECT_SOURCE_DIR}/pugixml.cpp
    ${PROJECT_SOURCE_DIR}/pugixml.hpp
    ${PROJECT_SOURCE_DIR}/pugiconfig.hpp
)

target_include_directories(PUGIXML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "DONE!")
