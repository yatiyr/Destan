add_library(destan_engine INTERFACE)

# Add all modules we need
target_link_libraries(destan_engine INTERFACE
    destan_core
    destan_physics
    destan_rendering
    destan_scripting
)

# Destan Engine module sees all engine submodule's header files
# so it can use in its own include file
target_include_directories(destan_engine INTERFACE
    $<TARGET_PROPERTY:destan_core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:destan_physics,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:destan_rendering,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:destan_scripting,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}
)