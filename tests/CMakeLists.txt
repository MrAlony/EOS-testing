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

# Unit tests (if we add a test framework later)
# add_executable(eos_unit_tests
#     unit_tests.cpp
# )
