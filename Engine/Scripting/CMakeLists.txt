# Scripting module configuration
set(MODULE_NAME destan_scripting)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(${MODULE_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        #Include'core module's includes
        $<TARGET_PROPERTY:destan_core,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC destan_core)

target_precompile_headers(destan_scripting REUSE_FROM destan_core)

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