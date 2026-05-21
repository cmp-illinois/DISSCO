cmake_minimum_required(VERSION 3.25)

project(CMOD)
message(STATUS "== CMOD ==")

find_package(SndFile REQUIRED)

file(GLOB_RECURSE CMOD_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

add_executable(CMOD ${CMOD_SRC})
# CMOD is consumed by LASSIE as a standalone subprocess (QProcess::start),
# not linked into it. No ENABLE_EXPORTS / WINDOWS_EXPORT_ALL_SYMBOLS is
# needed — and adding them just to "make the link work" backfires on
# Windows, where it requires an import library that no consumer actually uses.

### libsndfile must be linked in just the same way as it is in LASS ###
target_link_libraries(CMOD PUBLIC ${SNDFILE_LIBRARY})
target_link_libraries(CMOD PUBLIC PUGIXML)
# add_subdirectory(${CMAKE_SOURCE_DIR}/external-libs/muParser)
target_link_libraries(CMOD PUBLIC MUPARSER)
target_link_libraries(CMOD PUBLIC LASS)
message(STATUS "libsndfile, pugixml, muParser, and LASS includes linked with target!")

# heavy libraries that won't change much from compile to compile in this project
target_precompile_headers(CMOD PRIVATE
    <pugixml.hpp>
    <sndfile.h>
    <vector>
    <string>
    <map>
    <memory>
    <iostream>
    <fstream>
    <sstream>
)

### _external_ library checkers for informative errors ###
if(NOT SNDFILE_FOUND)
    message(SEND_ERROR "Couldn't find libsndfile, won't go well; check BUILDING docs")
else()
    message(STATUS "Found libsndfile install!")
endif()

# set "default" location of the CMOD binary (directory) for use in LASSIE
if(NOT CMOD_BINARY)
    set(CMOD_BINARY "${CMAKE_CURRENT_BINARY_DIR}/CMOD" PARENT_SCOPE)
    message(STATUS "CMOD_BINARY set")
endif()
message(STATUS "CMOD_BINARY set to ${CMOD_BINARY}")

include(GNUInstallDirs)
if(APPLE)
    install(TARGETS CMOD
        RUNTIME DESTINATION LASSIE.app/Contents/MacOS
        COMPONENT Runtime
    )
else()
    install(TARGETS CMOD
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()

message(STATUS "DONE!")
