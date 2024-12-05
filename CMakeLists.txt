cmake_minimum_required(VERSION 3.20)
project(destan_engine VERSION 0.1.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;StrictDebug;Release;Shipping" CACHE STRING "Configuration Types" FORCE)

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /DDESTAN_DEBUG")
    set(CMAKE_CXX_FLAGS_STRICTDEBUG "/Zi /Od /W4 /WX /permissive- /analyze /DDESTAN_DEBUG")    
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Zi /DDESTAN_RELEASE")
    set(CMAKE_CXX_FLAGS_SHIPPING "/O2 /DNDEBUG /DDESTAN_SHIPPING")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDESTAN_DEBUG")
    set(CMAKE_CXX_FLAGS_STRICTDEBUG "-g -O0 -Wall -Wextra -Wpedantic -Werror -DDESTAN_DEBUG")    
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -DDESTAN_RELEASE")
    set(CMAKE_CXX_FLAGS_SHIPPING "-O3 -DNDEBUG -DDESTAN_SHIPPING")
endif()

# C++ Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output directories
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

# Add main components of the engine
add_subdirectory(Engine)
add_subdirectory(Examples)