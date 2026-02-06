# Auth library
add_library(eos_auth STATIC
    auth_manager.cpp
)

target_include_directories(eos_auth PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(eos_auth PUBLIC eos_core)
