# Matchmaking library
add_library(eos_matchmaking STATIC
    matchmaking_manager.cpp
)

target_include_directories(eos_matchmaking PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(eos_matchmaking PUBLIC eos_core eos_auth eos_lobby)
