cmake_minimum_required(VERSION 3.25)

project(CMOD)
message(STATUS "== CMOD ==")

find_package(SndFile REQUIRED)

file(GLOB_RECURSE CMOD_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

list(REMOVE_ITEM CMOD_SRC
    "${PROJECT_SOURCE_DIR}/src/ModifierUsage.cpp"
    "${PROJECT_SOURCE_DIR}/src/ModifierUsage.hpp"
)

add_library(ModifierUsage STATIC
    src/ModifierUsage.cpp
    src/ModifierUsage.hpp
)
add_library(DISSCO::ModifierUsage ALIAS ModifierUsage)

target_include_directories(ModifierUsage PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)
target_compile_features(ModifierUsage PUBLIC cxx_std_20)

if(BUILD_TESTING)
    # Runs the real cmod program on the tutorial example projects and checks
    # the sound and particel it produces. -- Tony Chai, 2026
    add_executable(CmodExampleProjectsTests
        tests/CmodExampleProjectsTests.cpp
    )
    target_link_libraries(CmodExampleProjectsTests PRIVATE ${SNDFILE_LIBRARY})
    target_include_directories(CmodExampleProjectsTests PRIVATE ${SNDFILE_INCLUDE_DIR})
    target_compile_features(CmodExampleProjectsTests PRIVATE cxx_std_20)
    add_dependencies(CmodExampleProjectsTests CMOD)
    add_test(NAME CMOD.ExampleProjectsCompose
        COMMAND CmodExampleProjectsTests
            $<TARGET_FILE:CMOD>
            "${CMAKE_SOURCE_DIR}/Documents/examples/tutorial_sound"
    )
    set_tests_properties(CMOD.ExampleProjectsCompose PROPERTIES TIMEOUT 300)

    # Checks the functions composers type into LASSIE fields (RandomInt,
    # Select, Stochos, ...). CMOD is only built as a program, so this
    # compiles CMOD's sources (minus its main()) into the test. -- Tony Chai, 2026
    set(CMOD_TESTABLE_SRC ${CMOD_SRC})
    list(REMOVE_ITEM CMOD_TESTABLE_SRC "${PROJECT_SOURCE_DIR}/src/Main.cpp")
    add_executable(CmodFunctionTests
        tests/CmodFunctionTests.cpp
        ${CMOD_TESTABLE_SRC}
    )
    target_link_libraries(CmodFunctionTests PRIVATE
        ${SNDFILE_LIBRARY} PUGIXML MUPARSER LASS DISSCO::ModifierUsage)
    target_compile_features(CmodFunctionTests PRIVATE cxx_std_20)
    add_test(NAME CMOD.Functions COMMAND CmodFunctionTests)

    # Checks the functions that read from the event being built
    # (CURRENT_CHILD_NUM, GetPattern, ...) by running the real cmod program
    # on small generated projects and reading its particel. -- Tony Chai, 2026
    add_executable(CmodEventFunctionsTests
        tests/CmodEventFunctionsTests.cpp
    )
    target_compile_features(CmodEventFunctionsTests PRIVATE cxx_std_20)
    add_dependencies(CmodEventFunctionsTests CMOD)
    add_test(NAME CMOD.EventFunctions
        COMMAND CmodEventFunctionsTests $<TARGET_FILE:CMOD>
    )
    set_tests_properties(CMOD.EventFunctions PROPERTIES TIMEOUT 120)

    # Tag every test registered in this folder (including future ones) with
    # the "CMOD" label, so CI can run and report each part of DISSCO as its
    # own step (ctest -L '^CMOD$'). -- Tony Chai, 2026
    get_property(_cmod_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${_cmod_tests} PROPERTIES LABELS CMOD)
endif()

add_executable(CMOD ${CMOD_SRC})
set(CMOD_EXECUTABLE_BASENAME "cmod")
set_target_properties(CMOD PROPERTIES
    OUTPUT_NAME "${CMOD_EXECUTABLE_BASENAME}"
)
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
target_link_libraries(CMOD PUBLIC DISSCO::ModifierUsage)
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
    set(CMOD_BINARY
        "${CMAKE_CURRENT_BINARY_DIR}/${CMOD_EXECUTABLE_BASENAME}"
        PARENT_SCOPE
    )
    message(STATUS "CMOD_BINARY set")
endif()
message(STATUS "CMOD_BINARY set to ${CMOD_BINARY}")

include(GNUInstallDirs)
if(APPLE)
    install(TARGETS CMOD
        RUNTIME DESTINATION lassie.app/Contents/MacOS
        COMPONENT Runtime
    )
else()
    install(TARGETS CMOD
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()

# Ship libsndfile's runtime DLL alongside the executables on Windows when
# FindSndFile resolved to the FetchContent prebuilt (vcpkg / brew / apt
# installs put it in a system-discoverable place and don't need this).
# windeployqt only knows about Qt DLLs, so this is on us.
if(WIN32 AND SNDFILE_DLL)
    install(FILES "${SNDFILE_DLL}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()

message(STATUS "DONE!")
