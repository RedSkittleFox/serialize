cmake_minimum_required(VERSION 3.5)

cmake_policy(PUSH)

enable_testing()

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

if(NOT FOX_SERIALIZE_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG main
) 

set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googlebenchmark)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/fox_serialize_benchmark.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hand_written_benchmark.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_common.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_main.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    serialize-benchmark
    ${sources}
)

if(MSVC)
	target_compile_options(
	    serialize-benchmark
		PRIVATE /W4 
		PRIVATE /MP 
		PRIVATE /arch:AVX2
		PRIVATE /WX
	)
endif()

target_link_libraries(
    serialize-benchmark
    benchmark::benchmark
    serialize
)

if (PROJECT_IS_TOP_LEVEL)
    set_target_properties(gtest_main PROPERTIES FOLDER "vendor")
    set_target_properties(gtest PROPERTIES FOLDER "vendor")
    set_target_properties(gmock_main PROPERTIES FOLDER "vendor")
    set_target_properties(gmock PROPERTIES FOLDER "vendor")
    set_target_properties(benchmark_main PROPERTIES FOLDER "vendor")
    set_target_properties(benchmark PROPERTIES FOLDER "vendor")
endif()

cmake_policy(POP)