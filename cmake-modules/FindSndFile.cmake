# FindSndFile.cmake
#
# Origin: SuperCollider's FindSndFile.cmake (GPLv3). This file has been
# extended to handle modern installs that ship a CMake package-config
# (`SndFileConfig.cmake`) — notably vcpkg, recent Homebrew, and libsndfile
# 1.2+'s official Windows prebuilt zip — and to optionally fall back to
# downloading that prebuilt zip on Windows.
#
# Search order:
#   1. Already-set cache (SNDFILE_LIBRARY + SNDFILE_INCLUDE_DIR).
#   2. CONFIG-mode `find_package(SndFile)` — picks up vcpkg, Homebrew, the
#      official Windows prebuilt zip after FetchContent below, anything that
#      ships `SndFileConfig.cmake`.
#   3. Legacy MODULE-mode hunting via find_path / find_library — keeps old
#      Mega-Nerd Windows installs and unusual Unix layouts working.
#   4. (Windows only) FetchContent download of the official libsndfile
#      Windows zip from GitHub Releases, then re-try CONFIG mode against
#      the extracted tree. Gated by DISSCO_SNDFILE_FETCH_PREBUILT (ON by
#      default on Windows; OFF everywhere else, since brew / apt should
#      cover it).
#
# Outputs (set on success):
#   SNDFILE_FOUND        - True
#   SNDFILE_INCLUDE_DIR  - Directory containing sndfile.h
#   SNDFILE_LIBRARY      - Path to the link library
#   SNDFILE_LIBRARIES    - Same as SNDFILE_LIBRARY (list form, legacy var)
#   SNDFILE_DLL          - (Windows only, prebuilt path) Path to sndfile.dll
#                          that downstream `install()` rules can ship.

if(NO_LIBSNDFILE)
    set(SNDFILE_FOUND FALSE)
    set(SNDFILE_INCLUDE_DIR "nowhere")  # for onceonly check above
    set(SNDFILE_LIBRARIES "")
    add_definitions("-DNO_LIBSNDFILE")
    return()
endif()

# (1) Pre-set by user / parent project — trust it.
if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
    set(SNDFILE_LIBRARIES "${SNDFILE_LIBRARY}")
    set(SNDFILE_FOUND TRUE)
endif()

# Helper: pull paths out of an imported SndFile::sndfile target so legacy
# ${SNDFILE_LIBRARY} consumers downstream don't need to change.
function(_dissco_sndfile_extract_from_target)
    if(NOT TARGET SndFile::sndfile)
        return()
    endif()
    get_target_property(_inc SndFile::sndfile INTERFACE_INCLUDE_DIRECTORIES)
    # IMPORTED_LOCATION_<CONFIG> is the actual file; LOCATION may be a
    # generator expression. Try a few common configs.
    foreach(_cfg IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_RELWITHDEBINFO
                 IMPORTED_LOCATION_MINSIZEREL IMPORTED_LOCATION_DEBUG
                 IMPORTED_LOCATION)
        get_target_property(_loc SndFile::sndfile ${_cfg})
        if(_loc AND NOT _loc STREQUAL "_loc-NOTFOUND")
            break()
        endif()
    endforeach()
    if(_inc AND _loc)
        set(SNDFILE_INCLUDE_DIR "${_inc}" PARENT_SCOPE)
        set(SNDFILE_LIBRARY     "${_loc}" PARENT_SCOPE)
        set(SNDFILE_FOUND       TRUE      PARENT_SCOPE)
    endif()
endfunction()

# (2) CONFIG mode — vcpkg, modern brew, prebuilt zip.
if(NOT SNDFILE_FOUND)
    find_package(SndFile CONFIG QUIET)
    if(SndFile_FOUND)
        _dissco_sndfile_extract_from_target()
        if(SNDFILE_FOUND)
            message(STATUS "FindSndFile: using CONFIG-mode SndFile::sndfile target (${SNDFILE_LIBRARY})")
        endif()
    endif()
endif()

# (3) Legacy MODULE-mode path hunting (the original SuperCollider code).
if(NOT SNDFILE_FOUND)
    if(APPLE)
        find_path(SNDFILE_INCLUDE_DIR sndfile.h
            HINTS /usr/local/opt/libsndfile/include
        )
        find_library(SNDFILE_LIBRARY NAMES libsndfile.dylib libsndfile.a
            HINTS /usr/local/opt/libsndfile/lib
        )
        if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
            set(SNDFILE_FOUND TRUE)
            set(SNDFILE_LIBRARIES "${SNDFILE_LIBRARY}")
            message(STATUS "FindSndFile: legacy MODULE-mode find on macOS — ${SNDFILE_LIBRARY}")
        endif()
    else()
        find_path(SNDFILE_INCLUDE_DIR sndfile.h
            HINTS
                "${CMAKE_SOURCE_DIR}/../${CMAKE_LIBRARY_ARCHITECTURE}/libsndfile/include"
                "$ENV{ProgramW6432}/Mega-Nerd/libsndfile/include"
                "$ENV{ProgramFiles}/Mega-Nerd/libsndfile/include"
                "$ENV{PROGRAMFILES\(X86\)}/libsndfile/include"
                "$ENV{ProgramFiles}/libsndfile/include"
            PATHS /usr/local/include /usr/include
        )
        find_library(SNDFILE_LIBRARY
            NAMES sndfile sndfile-1 libsndfile libsndfile-1
            HINTS
                "${CMAKE_SOURCE_DIR}/../${CMAKE_LIBRARY_ARCHITECTURE}/libsndfile/lib"
                "${CMAKE_SOURCE_DIR}/../${CMAKE_LIBRARY_ARCHITECTURE}/libsndfile/bin"
                "$ENV{ProgramW6432}/Mega-Nerd/libsndfile/lib"
                "$ENV{ProgramW6432}/Mega-Nerd/libsndfile/bin"
                "$ENV{ProgramFiles}/Mega-Nerd/libsndfile/lib"
                "$ENV{ProgramFiles}/Mega-Nerd/libsndfile/bin"
                "$ENV{PROGRAMFILES\(X86\)}/libsndfile/lib"
                "$ENV{PROGRAMFILES\(X86\)}/libsndfile/bin"
                "$ENV{ProgramFiles}/libsndfile/lib"
                "$ENV{ProgramFiles}/libsndfile/bin"
            PATHS /usr/local /usr/lib
        )
        if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
            set(SNDFILE_FOUND TRUE)
            set(SNDFILE_LIBRARIES "${SNDFILE_LIBRARY}")
            message(STATUS "FindSndFile: legacy MODULE-mode find — ${SNDFILE_LIBRARY}")
        endif()
    endif()
endif()

# (4) Windows: download the official prebuilt zip from libsndfile's GitHub
# Releases as a last resort. The unpacked tree includes a proper
# SndFileConfig.cmake; we point CMake at it and re-enter CONFIG mode.
option(DISSCO_SNDFILE_FETCH_PREBUILT
    "On Windows, download libsndfile's official prebuilt zip when no local install is found."
    ON)

if(NOT SNDFILE_FOUND AND WIN32 AND DISSCO_SNDFILE_FETCH_PREBUILT)
    # Pinned to libsndfile 1.2.2's official win64 release. SHA verified
    # against the asset on the GitHub Releases page. Bump deliberately.
    set(_sndfile_url "https://github.com/libsndfile/libsndfile/releases/download/1.2.2/libsndfile-1.2.2-win64.zip")
    set(_sndfile_sha "2173935c0c1ed13cf627951d34483f9d405ead2eb473190461c42ba220643a3f")

    include(FetchContent)
    FetchContent_Declare(libsndfile_prebuilt
        URL      "${_sndfile_url}"
        URL_HASH "SHA256=${_sndfile_sha}"
    )
    FetchContent_MakeAvailable(libsndfile_prebuilt)

    # The zip extracts into libsndfile-1.2.2-win64/* — find that root.
    file(GLOB _sndfile_roots LIST_DIRECTORIES TRUE
        "${libsndfile_prebuilt_SOURCE_DIR}/libsndfile-*-win64"
        "${libsndfile_prebuilt_SOURCE_DIR}")
    foreach(_root ${_sndfile_roots})
        if(EXISTS "${_root}/cmake/SndFileConfig.cmake")
            set(SndFile_DIR "${_root}/cmake" CACHE PATH "" FORCE)
            break()
        endif()
    endforeach()

    find_package(SndFile CONFIG QUIET)
    if(SndFile_FOUND)
        _dissco_sndfile_extract_from_target()
        # Expose the DLL so install() rules can ship it next to LASSIE.exe.
        find_file(SNDFILE_DLL
            NAMES sndfile.dll libsndfile-1.dll
            HINTS "${_root}/bin"
            NO_DEFAULT_PATH
        )
        message(STATUS "FindSndFile: using FetchContent prebuilt — ${SNDFILE_LIBRARY}")
        if(SNDFILE_DLL)
            message(STATUS "FindSndFile: runtime DLL at ${SNDFILE_DLL}")
        endif()
    endif()
endif()

mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY SNDFILE_DLL)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SndFile
    REQUIRED_VARS SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR
)

# --- Format-support feature probes (carried over from the SC original) ----
# These ask the libsndfile headers whether they declare the format constants
# — they only check the version, not whether the codec was actually built in.

if(SNDFILE_FOUND)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_INCLUDES ${SNDFILE_INCLUDE_DIR})

    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <sndfile.h>
        int main(void) { return SF_FORMAT_VORBIS; }
        " SNDFILE_HAS_VORBIS)
    check_c_source_compiles("
        #include <sndfile.h>
        int main(void) { return SF_FORMAT_OPUS; }
        " SNDFILE_HAS_OPUS)
    check_c_source_compiles("
        #include <sndfile.h>
        int main(void) { return SF_FORMAT_MPEG; }
        " SNDFILE_HAS_MPEG)

    cmake_pop_check_state()
endif()
