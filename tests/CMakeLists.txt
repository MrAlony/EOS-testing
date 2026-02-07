# Test applications
add_executable(eos_test_client
    test_client.cpp
)

target_link_libraries(eos_test_client PRIVATE
    eos_core
    eos_auth
    eos_lobby
    eos_p2p
    eos_voice
    eos_matchmaking
)

# P2P Host/Client test executables
add_executable(eos_host
    host.cpp
)

target_link_libraries(eos_host PRIVATE
    eos_core
    eos_auth
    eos_lobby
    eos_p2p
    eos_voice
)

add_executable(eos_client
    client.cpp
)

target_link_libraries(eos_client PRIVATE
    eos_core
    eos_auth
    eos_lobby
    eos_p2p
    eos_voice
)

# Unit tests (if we add a test framework later)
# add_executable(eos_unit_tests
#     unit_tests.cpp
# )
