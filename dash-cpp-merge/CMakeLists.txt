cmake_minimum_required(VERSION 3.10)
project(dash-cpp-merge VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Debug mode option (OFF by default)
option(DEBUG "Enable debug output" OFF)
if(DEBUG)
    add_definitions(-DDEBUG)
endif()

# Readline option (ON by default)
option(USE_READLINE "Enable readline support" ON)
if(USE_READLINE)
    add_definitions(-DREADLINE_ENABLED)
endif()

# Find required libraries
find_package(Threads REQUIRED)
if(USE_READLINE)
    include(cmake/FindReadline.cmake)
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Add readline include path if found
if(USE_READLINE AND READLINE_FOUND)
    include_directories(${READLINE_INCLUDE_DIR})
endif()

# Source files
file(GLOB_RECURSE SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.c
)

# Header files
file(GLOB_RECURSE HEADERS 
    ${PROJECT_SOURCE_DIR}/include/*.h
)

# Executable
add_executable(dash ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(dash 
    ${CMAKE_THREAD_LIBS_INIT}
)

# Add readline library if found
if(USE_READLINE AND READLINE_FOUND)
    target_link_libraries(dash ${READLINE_LIBRARY})
endif()

# Installation
install(TARGETS dash DESTINATION bin)

# Tests (only if the directory exists and contains a CMakeLists.txt)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
else()
    message(STATUS "Tests directory not found or no CMakeLists.txt in tests directory. Skipping tests.")
endif() 