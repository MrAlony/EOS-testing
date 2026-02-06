cmake_minimum_required(VERSION 3.20)
project(EOS_Testing 
    LANGUAGES CXX
    DESCRIPTION "EOS Epic Relay Service Test Project - Proving P2P/Relay for multiplayer games"
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(EOS_BUILD_TESTS "Build test applications" ON)
option(EOS_BUILD_EXAMPLES "Build example applications" ON)

# Platform detection
if(WIN32)
    set(EOS_PLATFORM "Windows")
    add_definitions(-DEOS_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(EOS_PLATFORM "Linux")
    add_definitions(-DEOS_PLATFORM_LINUX)
elseif(APPLE)
    set(EOS_PLATFORM "Mac")
    add_definitions(-DEOS_PLATFORM_MAC)
endif()

message(STATUS "Building for platform: ${EOS_PLATFORM}")

# EOS SDK Path - User must set this or place SDK in ./external/eos-sdk
set(EOS_SDK_PATH "${CMAKE_SOURCE_DIR}/external/eos-sdk" CACHE PATH "Path to EOS SDK")

if(EXISTS "${EOS_SDK_PATH}/Include/eos_sdk.h")
    message(STATUS "EOS SDK found at: ${EOS_SDK_PATH}")
    set(EOS_SDK_FOUND TRUE)
else()
    message(WARNING "EOS SDK not found at ${EOS_SDK_PATH}. Building in stub mode.")
    message(WARNING "Download EOS SDK from: https://dev.epicgames.com/portal")
    set(EOS_SDK_FOUND FALSE)
    add_definitions(-DEOS_STUB_MODE)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

if(EOS_SDK_FOUND)
    include_directories(${EOS_SDK_PATH}/Include)
    
    # Link EOS SDK libraries based on platform
    if(WIN32)
        set(EOS_SDK_LIB "${EOS_SDK_PATH}/Lib/EOSSDK-Win64-Shipping.lib")
        set(EOS_SDK_DLL "${EOS_SDK_PATH}/Bin/EOSSDK-Win64-Shipping.dll")
    elseif(UNIX AND NOT APPLE)
        set(EOS_SDK_LIB "${EOS_SDK_PATH}/Bin/libEOSSDK-Linux-Shipping.so")
    elseif(APPLE)
        set(EOS_SDK_LIB "${EOS_SDK_PATH}/Bin/libEOSSDK-Mac-Shipping.dylib")
    endif()
endif()

# Core library
add_subdirectory(src/core)

# Module libraries
add_subdirectory(src/auth)
add_subdirectory(src/lobby)
add_subdirectory(src/p2p)
add_subdirectory(src/voice)
add_subdirectory(src/matchmaking)

# Test applications
if(EOS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Example client
if(EOS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Copy DLL to output on Windows
if(WIN32 AND EOS_SDK_FOUND AND EXISTS ${EOS_SDK_DLL})
    add_custom_target(copy_eos_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EOS_SDK_DLL}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()
