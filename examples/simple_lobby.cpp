/**
 * EOS Testing - Simple Lobby Example
 * 
 * Demonstrates a minimal lobby + P2P setup for a multiplayer game.
 * This is the pattern you'd use for a Crab Game-style party game.
 */

#include "eos_testing/eos_testing.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace eos_testing;

// Simple game state
struct GameState {
    std::atomic<bool> running{true};
    std::atomic<bool> in_lobby{false};
    std::atomic<bool> game_started{false};
};

GameState g_state;

void setup_callbacks() {
    auto& lobby = LobbyManager::instance();
    auto& p2p = P2PManager::instance();
    
    // Lobby callbacks
    lobby.on_member_join = [](const std::string& lobby_id, const LobbyMember& member) {
        std::cout << "[LOBBY] Player joined: " << member.display_name << "\n";
    };
    
    lobby.on_member_leave = [](const std::string& lobby_id, EOS_ProductUserId user_id) {
        std::cout << "[LOBBY] Player left\n";
    };
    
    lobby.on_lobby_updated = [](const LobbyInfo& lobby) {
        std::cout << "[LOBBY] Updated - " << lobby.current_members << "/" 
                  << lobby.max_members << " players\n";
    };
    
    // P2P callbacks
    p2p.on_connection_established = [](EOS_ProductUserId peer, ConnectionStatus status) {
        std::cout << "[P2P] Connected to peer\n";
    };
    
    p2p.on_connection_closed = [](EOS_ProductUserId peer, ConnectionStatus status) {
        std::cout << "[P2P] Disconnected from peer\n";
    };
    
    p2p.on_packet_received = [](const IncomingPacket& packet) {
        // Handle game packets here
        std::cout << "[P2P] Received " << packet.data.size() << " bytes\n";
    };
}

void host_game() {
    std::cout << "\n--- Hosting a new game ---\n";
    
    auto& auth = AuthManager::instance();
    auto& lobby = LobbyManager::instance();
    auto& p2p = P2PManager::instance();
    
    // Login
    auth.login_device_id("HostPlayer", [](const AuthResult& result) {
        if (result.success) {
            std::cout << "Logged in as: " << result.display_name << "\n";
        }
    });
    
    // Initialize P2P
    P2PConfig p2p_config;
    p2p_config.socket_name = "CrabGameSocket";
    p2p.initialize(p2p_config);
    p2p.accept_connections();
    
    // Create lobby
    CreateLobbyOptions options;
    options.lobby_name = "Fun Party Game!";
    options.max_members = 10;
    options.attributes["game_mode"] = "classic";
    
    lobby.create_lobby(options, [](bool success, const std::string& lobby_id, const std::string& error) {
        if (success) {
            std::cout << "Lobby created! ID: " << lobby_id << "\n";
            std::cout << "Waiting for players to join...\n";
            g_state.in_lobby = true;
        }
    });
}

void join_game(const std::string& lobby_id) {
    std::cout << "\n--- Joining game: " << lobby_id << " ---\n";
    
    auto& auth = AuthManager::instance();
    auto& lobby = LobbyManager::instance();
    auto& p2p = P2PManager::instance();
    
    // Login
    auth.login_device_id("JoinPlayer", [](const AuthResult& result) {
        if (result.success) {
            std::cout << "Logged in as: " << result.display_name << "\n";
        }
    });
    
    // Initialize P2P
    P2PConfig p2p_config;
    p2p_config.socket_name = "CrabGameSocket";
    p2p.initialize(p2p_config);
    p2p.accept_connections();
    
    // Join lobby
    lobby.join_lobby(lobby_id, [&p2p](bool success, const LobbyInfo& info, const std::string& error) {
        if (success) {
            std::cout << "Joined lobby: " << info.lobby_name << "\n";
            g_state.in_lobby = true;
            
            // Connect to all lobby members via P2P
            for (const auto& member : info.members) {
                if (member.user_id != AuthManager::instance().get_product_user_id()) {
                    p2p.connect_to_peer(member.user_id);
                }
            }
        }
    });
}

void game_loop() {
    auto& p2p = P2PManager::instance();
    
    while (g_state.running) {
        // Tick EOS
        Platform::instance().tick();
        
        // Process incoming packets
        p2p.receive_packets();
        
        // If game started, send position updates
        if (g_state.game_started) {
            // Example: send player position
            struct PlayerUpdate {
                float x, y, z;
                float rotation;
            } update = {0.0f, 0.0f, 0.0f, 0.0f};
            
            p2p.broadcast_packet(&update, sizeof(update), 0, 
                                  PacketReliability::UnreliableUnordered);
        }
        
        // Sleep to maintain ~60 ticks per second
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=================================\n";
    std::cout << "   Simple Lobby Example\n";
    std::cout << "=================================\n\n";
    
    // Initialize EOS
    PlatformConfig config;
    config.product_name = "Simple Lobby Example";
    config.product_version = "1.0.0";
    config.product_id = "your_product_id";
    config.sandbox_id = "your_sandbox_id";
    config.deployment_id = "your_deployment_id";
    config.client_id = "your_client_id";
    config.client_secret = "your_client_secret";
    
    eos_testing::initialize(config, [](bool success, const std::string& msg) {
        std::cout << "EOS Init: " << msg << "\n";
    });
    
    setup_callbacks();
    
    // Decide whether to host or join
    if (argc > 1) {
        // Join existing lobby
        join_game(argv[1]);
    } else {
        // Host new lobby
        host_game();
    }
    
    // Run for a few seconds to demonstrate
    std::cout << "\nRunning game loop for 3 seconds...\n";
    
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        Platform::instance().tick();
        P2PManager::instance().receive_packets();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    std::cout << "\nShutting down...\n";
    LobbyManager::instance().leave_lobby(nullptr);
    P2PManager::instance().shutdown();
    eos_testing::shutdown();
    
    std::cout << "Done!\n";
    return 0;
}
