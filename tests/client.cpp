/**
 * EOS Testing - Client Application
 * 
 * Searches for a lobby, joins it, and establishes P2P connection with host.
 * Responds to pings with pongs.
 * 
 * Usage: eos_client.exe
 */

#include "eos_testing/eos_testing.hpp"
#include "../config/credentials.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

using namespace eos_testing;

std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

// Simple packet types for our test (must match host)
enum class PacketType : uint8_t {
    Ping = 1,
    Pong = 2,
    Chat = 3
};

struct TestPacket {
    PacketType type;
    uint32_t sequence;
    char message[256];
};

int main() {
    std::signal(SIGINT, signal_handler);
    
    std::cout << "==============================================\n";
    std::cout << "        EOS P2P Test - CLIENT MODE\n";
    std::cout << "==============================================\n\n";
    
    // Initialize platform
    std::cout << "[CLIENT] Initializing EOS Platform...\n";
    
    PlatformConfig config;
    config.product_name = config::PRODUCT_NAME;
    config.product_version = config::PRODUCT_VERSION;
    config.product_id = config::PRODUCT_ID;
    config.sandbox_id = config::SANDBOX_ID;
    config.deployment_id = config::DEPLOYMENT_ID;
    config.client_id = config::CLIENT_ID;
    config.client_secret = config::CLIENT_SECRET;
    
    bool init_done = false;
    eos_testing::initialize(config, [&](bool success, const std::string& msg) {
        if (success) {
            std::cout << "[CLIENT] Platform initialized!\n";
        } else {
            std::cout << "[CLIENT] Platform init failed: " << msg << "\n";
            g_running = false;
        }
        init_done = true;
    });
    
    while (!init_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    if (!Platform::instance().is_ready()) {
        return 1;
    }
    
    // Login with Device ID (different display name)
    std::cout << "[CLIENT] Logging in...\n";
    
    bool login_done = false;
    EOS_ProductUserId my_user_id = nullptr;
    
    // IMPORTANT: Delete existing device ID to create new identity (separate from host on same machine)
    AuthManager::instance().login_device_id_with_model("Client", "ClientPC", true, [&](const AuthResult& result) {
        if (result.success) {
            std::cout << "[CLIENT] Logged in as '" << result.display_name << "'\n";
            std::cout << "[CLIENT] User ID: " << result.product_user_id << "\n";
            my_user_id = AuthManager::instance().get_product_user_id();
        } else {
            std::cout << "[CLIENT] Login failed: " << result.error_message << "\n";
            g_running = false;
        }
        login_done = true;
    });
    
    while (!login_done && g_running) {
        Platform::instance().tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    if (!g_running) return 1;
    
    // Initialize P2P
    std::cout << "[CLIENT] Initializing P2P...\n";
    
    P2PConfig p2p_config;
    p2p_config.socket_name = "P2PTestSocket";  // Must match host
    p2p_config.allow_relay = true;
    
    if (!P2PManager::instance().initialize(p2p_config)) {
        std::cout << "[CLIENT] P2P init failed!\n";
        return 1;
    }
    
    // Track connected host
    EOS_ProductUserId connected_host = nullptr;
    uint32_t pings_received = 0;
    uint32_t pongs_sent = 0;
    
    // Set up P2P callbacks
    P2PManager::instance().on_connection_established = [&](EOS_ProductUserId peer, ConnectionStatus status) {
        if (status == ConnectionStatus::Connected) {
            std::cout << "[CLIENT] Connected to host via P2P!\n";
            connected_host = peer;
            
            // Send initial chat message
            TestPacket chat;
            chat.type = PacketType::Chat;
            chat.sequence = 0;
            snprintf(chat.message, sizeof(chat.message), "Hello from client!");
            P2PManager::instance().send_packet(peer, &chat, sizeof(chat), 0, PacketReliability::ReliableOrdered);
        }
    };
    
    P2PManager::instance().on_connection_closed = [&](EOS_ProductUserId peer, ConnectionStatus status) {
        std::cout << "[CLIENT] Disconnected from host.\n";
        if (peer == connected_host) {
            connected_host = nullptr;
        }
    };
    
    P2PManager::instance().on_packet_received = [&](const IncomingPacket& packet) {
        if (packet.data.size() >= sizeof(TestPacket)) {
            TestPacket* pkt = (TestPacket*)packet.data.data();
            
            switch (pkt->type) {
                case PacketType::Ping: {
                    pings_received++;
                    std::cout << "[CLIENT] Received PING #" << pkt->sequence << "\n";
                    
                    // Send pong back
                    TestPacket pong;
                    pong.type = PacketType::Pong;
                    pong.sequence = pkt->sequence;
                    snprintf(pong.message, sizeof(pong.message), "Pong!");
                    
                    if (P2PManager::instance().send_packet(packet.sender, &pong, sizeof(pong), 
                                                            0, PacketReliability::ReliableOrdered)) {
                        pongs_sent++;
                        std::cout << "[CLIENT] Sent PONG #" << pong.sequence << "\n";
                    }
                    break;
                }
                    
                case PacketType::Chat:
                    std::cout << "[CLIENT] Host says: " << pkt->message << "\n";
                    break;
                    
                default:
                    break;
            }
        }
    };
    
    // Accept incoming connections (host might connect to us)
    P2PManager::instance().accept_connections();
    std::cout << "[CLIENT] Accepting P2P connections...\n";
    
    // Search for host's lobby
    std::cout << "[CLIENT] Searching for lobbies...\n";
    
    bool search_done = false;
    bool found_lobby = false;
    std::string target_lobby_id;
    EOS_ProductUserId host_user_id = nullptr;
    
    // Search with the same bucket_id as host
    std::string bucket_id = "p2ptest:global";  // Must match host's bucket_id
    std::unordered_map<std::string, std::string> filters;
    // Empty filters = find all lobbies in bucket
    
    LobbyManager::instance().search_lobbies(bucket_id, 10, filters, [&](bool success, const std::vector<LobbySearchResult>& results) {
        if (success && !results.empty()) {
            std::cout << "[CLIENT] Found " << results.size() << " lobby(ies):\n";
            for (const auto& lobby : results) {
                std::cout << "  - " << lobby.lobby_name << " (" << lobby.current_members << "/" << lobby.max_members << ")\n";
            }
            
            // Join the first one
            target_lobby_id = results[0].lobby_id;
            found_lobby = true;
        } else {
            std::cout << "[CLIENT] No lobbies found. Make sure host is running!\n";
        }
        search_done = true;
    });
    
    // Wait for search with timeout
    int search_attempts = 0;
    while (!found_lobby && g_running && search_attempts < 30) {  // 30 second timeout
        while (!search_done && g_running) {
            Platform::instance().tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        if (!found_lobby && g_running) {
            std::cout << "[CLIENT] Retrying search...\n";
            search_done = false;
            search_attempts++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            LobbyManager::instance().search_lobbies(bucket_id, 10, filters, [&](bool success, const std::vector<LobbySearchResult>& results) {
                if (success && !results.empty()) {
                    target_lobby_id = results[0].lobby_id;
                    found_lobby = true;
                    std::cout << "[CLIENT] Found lobby: " << results[0].lobby_name << "\n";
                }
                search_done = true;
            });
        }
    }
    
    if (!found_lobby) {
        std::cout << "[CLIENT] Could not find host lobby. Exiting.\n";
        eos_testing::shutdown();
        return 1;
    }
    
    // Join the lobby
    std::cout << "[CLIENT] Joining lobby: " << target_lobby_id << "\n";
    
    bool join_done = false;
    LobbyManager::instance().join_lobby(target_lobby_id, [&](bool success, const LobbyInfo& lobby, const std::string& error) {
        if (success) {
            std::cout << "[CLIENT] Joined lobby!\n";
            std::cout << "[CLIENT] Host: " << (lobby.owner_id ? "found" : "unknown") << "\n";
            
            // Connect P2P to the host (owner)
            if (lobby.owner_id) {
                host_user_id = lobby.owner_id;
                std::cout << "[CLIENT] Connecting P2P to host...\n";
                P2PManager::instance().connect_to_peer(lobby.owner_id);
            }
            
            // Also try connecting to all members
            for (const auto& member : lobby.members) {
                if (member.user_id != my_user_id) {
                    std::cout << "[CLIENT] Found member: " << member.display_name << "\n";
                    P2PManager::instance().connect_to_peer(member.user_id);
                }
            }
        } else {
            std::cout << "[CLIENT] Failed to join lobby: " << error << "\n";
            g_running = false;
        }
        join_done = true;
    });
    
    while (!join_done && g_running) {
        Platform::instance().tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Main loop
    std::cout << "\n[CLIENT] Running... Press Ctrl+C to stop.\n";
    std::cout << "[CLIENT] Waiting for pings from host...\n\n";
    
    while (g_running) {
        Platform::instance().tick();
        P2PManager::instance().receive_packets();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    std::cout << "\n[CLIENT] Shutting down...\n";
    std::cout << "[CLIENT] Stats: " << pings_received << " pings received, " << pongs_sent << " pongs sent\n";
    
    LobbyManager::instance().leave_lobby(nullptr);
    P2PManager::instance().shutdown();
    eos_testing::shutdown();
    
    std::cout << "[CLIENT] Done.\n";
    return 0;
}
