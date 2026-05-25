cmake_minimum_required(VERSION 3.25)

string(ASCII 27 Esc)
set(TextReset "${Esc}[m")
set(TextBold "${Esc}[1m")

### VERY IMPORTANT. USE OUT OF SOURCE BUILDS ONLY! ###
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR}) 
    message(FATAL_ERROR "In-source builds are prohibited.\n"
            "Create a subdirectory 'build/' and use 'cmake ..' inside it.\n"
            "${TextBold}Delete 'CMakeCache.txt' and 'CMakeFiles/' before you continue.${TextReset}")
    
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

# will define constants related to the version info
include(DISSCOVersions.txt)

project(DISSCO VERSION ${DISSCO_VERSION})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(LLD_LINKER ld.lld)
    if(LLD_LINKER)
        add_link_options(-fuse-ld=lld)
        message(STATUS "Using lld linker")
    endif()
endif()

option(BUILD_LASS_EXAMPLES "Build LASS examples" OFF)

# see Cross Compiling with CMake -- Mastering CMake for more on the eponymous subject
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchains/windows.cmake")
elseif(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-deprecated")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchains/macos.cmake")
elseif(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-deprecated")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchains/linux.cmake")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/LASS)
add_subdirectory(${CMAKE_SOURCE_DIR}/external-libs)
add_subdirectory(${CMAKE_SOURCE_DIR}/CMOD) # if successful, will set a CMOD_BINARY_DIR variable in the scope of _this_ file
add_subdirectory(${CMAKE_SOURCE_DIR}/LASSIE)

include(${CMAKE_SOURCE_DIR}/packaging/Packaging.cmake)
