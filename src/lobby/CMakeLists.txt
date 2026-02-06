# Lobby library
add_library(eos_lobby STATIC
    lobby_manager.cpp
)

target_include_directories(eos_lobby PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(eos_lobby PUBLIC eos_core eos_auth)
