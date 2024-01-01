cmake_minimum_required(VERSION 3.21)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    serialize-demo
    ${sources}
)

if(${IPO_SUPPORTED})
    set_target_properties(serialize-demo PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(MSVC)
    target_compile_options(
	serialize-demo
        PRIVATE /WX # all warnings as errors 
        PRIVATE /MP # multi-processor compilation
        # PRIVATE /FR
    )
endif()

target_include_directories(
    serialize-demo
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    serialize-demo
    serialize
)