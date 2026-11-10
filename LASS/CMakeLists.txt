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

# CUDA is optional at build time and selected again at render time. Builds on
# machines without nvcc retain the existing CPU filter path.
option(LASS_ENABLE_CUDA "Build the optional CUDA biquad filter" ON)
if(LASS_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        find_package(CUDAToolkit QUIET)
    endif()
endif()

### in the future, we may fork LASS and let it be shared if we find a viable LASS install
add_library(LASS STATIC ${LASS_SRC})

if(LASS_ENABLE_CUDA AND CMAKE_CUDA_COMPILER AND CUDAToolkit_FOUND)
    target_sources(LASS PRIVATE CUDA/BiquadCuda.cu)
    target_compile_definitions(LASS PRIVATE LASS_WITH_CUDA=1)
    target_link_libraries(LASS PUBLIC CUDA::cudart_static)
    set_target_properties(LASS PROPERTIES
        CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON
        CUDA_RUNTIME_LIBRARY Static)
    message(STATUS "CUDA biquad filtering enabled (runtime GPU detection)")

    if(BUILD_TESTING)
        add_executable(lass-cuda-biquad-test CUDA/BiquadCudaTest.cpp)
        target_link_libraries(lass-cuda-biquad-test PRIVATE LASS)
        target_include_directories(lass-cuda-biquad-test PRIVATE "${PROJECT_SOURCE_DIR}/CUDA")
        add_test(NAME lass-cuda-biquad COMMAND lass-cuda-biquad-test)
        set_tests_properties(lass-cuda-biquad PROPERTIES SKIP_RETURN_CODE 77)
    endif()
elseif(LASS_ENABLE_CUDA)
    message(STATUS "CUDA compiler or toolkit unavailable; using CPU filters")
endif()

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
