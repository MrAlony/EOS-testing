# P2P library
add_library(eos_p2p STATIC
    p2p_manager.cpp
)

target_include_directories(eos_p2p PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(eos_p2p PUBLIC eos_core eos_auth)
