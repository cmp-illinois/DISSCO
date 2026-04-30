cmake_minimum_required(VERSION 3.25)

project(LASS)
message(STATUS "== LASS ==")

find_package(SndFile REQUIRED)

file(GLOB_RECURSE LASS_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

### not exactly sure how we want to deal with these, so disabled for now -Jacob ###
# file(GLOB_RECURSE EXAMPLES_SRC
#     "${EXAMPLES_DIR}/*.cpp"
#     "${EXAMPLES_DIR}/*.cxx"
# )

### in the future, we may fork LASS and let it be shared if we find a viable LASS install
add_library(LASS STATIC ${LASS_SRC})

### libsndfile must be linked in just the same way as it is in CMOD ###
target_link_libraries(LASS PUBLIC ${SNDFILE_LIBRARY})
message(STATUS "libsndfile linked with target!")

if(APPLE)
    target_link_libraries(LASS PUBLIC "-framework Accelerate")
    message(STATUS "Accelerate framework linked for vDSP reverb optimisation!")
endif()

target_include_directories(LASS PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    ${SNDFILE_INCLUDE_DIR}
)
message(STATUS "libsndfile included in target!")

### _external_ library checker for informative errors ###
if(NOT SNDFILE_FOUND)
    message(SEND_ERROR "Couldn't find libsndfile, won't go well; check BUILDING docs")
else()
    message(STATUS "Found libsndfile install!")
endif()

message(STATUS "DONE!")