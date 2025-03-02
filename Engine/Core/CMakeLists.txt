# Core module configuration
set(MODULE_NAME destan_core)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(${MODULE_NAME} STATIC ${SOURCES} ${HEADERS})

target_precompile_headers(${MODULE_NAME} 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core/destan_pch.h
)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${MODULE_NAME}
    PUBLIC
        $<$<CONFIG:Debug>:DESTAN_DEBUG>
        $<$<CONFIG:StrictDebug>:DESTAN_DEBUG>
        $<$<CONFIG:Release>:DESTAN_RELEASE>
        $<$<CONFIG:Shipping>:DESTAN_SHIPPING>
)

if(WIN32)
    target_compile_definitions(${MODULE_NAME} PUBLIC DESTAN_PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(${MODULE_NAME} PUBLIC DESTAN_PLATFORM_LINUX)
endif()