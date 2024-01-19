cmake_minimum_required(VERSION 3.5)

cmake_policy(PUSH)

enable_testing()

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/serialize_test.cc"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    serialize-test
    ${sources}
)

if(MSVC)
	target_compile_options(
	        serialize-test
		PRIVATE /W4 
		PRIVATE /MP 
		PRIVATE /arch:AVX2
		PRIVATE /WX
	)

endif()

target_link_libraries(
    serialize-test
    GTest::gtest_main
    GTest::gmock_main
    serialize
)

include(GoogleTest)
gtest_discover_tests(serialize-test)

if (PROJECT_IS_TOP_LEVEL)
    set_target_properties(gtest_main PROPERTIES FOLDER "vendor")
    set_target_properties(gtest PROPERTIES FOLDER "vendor")
    set_target_properties(gmock_main PROPERTIES FOLDER "vendor")
    set_target_properties(gmock PROPERTIES FOLDER "vendor")
endif()

cmake_policy(POP)