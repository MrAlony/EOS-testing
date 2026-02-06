# Example applications
add_executable(eos_simple_lobby
    simple_lobby.cpp
)

target_link_libraries(eos_simple_lobby PRIVATE
    eos_core
    eos_auth
    eos_lobby
    eos_p2p
)
