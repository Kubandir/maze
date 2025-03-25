cmake_minimum_required(VERSION 3.10)
project(maze VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add compiler options based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -g
        -O0
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
        -flto
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -ffast-math
        -ftree-vectorize
        -funroll-loops
        -fpeel-loops
        -fomit-frame-pointer
    )
    # Strip symbols in release mode
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--gc-sections -Wl,--strip-all -s")
endif()

# Enable Link Time Optimization for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO/LTO not supported: ${error}")
    endif()
endif()

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
