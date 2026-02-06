# Voice library
add_library(eos_voice STATIC
    voice_manager.cpp
)

target_include_directories(eos_voice PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(eos_voice PUBLIC eos_core eos_auth)
