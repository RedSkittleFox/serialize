cmake_minimum_required(VERSION 3.21)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/fox/serialize.hpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(
    serialize
    INTERFACE
    ${sources}
)

if(FOX_SERIALIZE_INCLUDE_REFLEXPR)
    include(FetchContent)

    FetchContent_Declare(
        fox_reflexpr
        GIT_REPOSITORY https://github.com/RedSkittleFox/reflexpr.git
        GIT_TAG main
    )
    set(FOX_REFLEXPR_BUILD_SAMPLES OFF)
    set(FOX_REFLEXPR_BUILD_TESTS OFF)	
    FetchContent_MakeAvailable(fox_reflexpr)
    
    target_compile_definitions(
        serialize
        INTERFACE
        FOX_SERIALIZE_HAS_REFLEXPR
    )
    
    target_link_libraries(
        serialize
        INTERFACE
        fox::reflexpr
    )
endif()


target_include_directories(
    serialize
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)