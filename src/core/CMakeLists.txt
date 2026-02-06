# Core library
add_library(eos_core STATIC
    platform.cpp
)

target_include_directories(eos_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(EOS_SDK_FOUND)
    target_include_directories(eos_core PUBLIC ${EOS_SDK_PATH}/Include)
    target_link_libraries(eos_core PUBLIC ${EOS_SDK_LIB})
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(eos_core PUBLIC ws2_32 winmm)
endif()
