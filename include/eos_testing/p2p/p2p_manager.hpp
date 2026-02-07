#pragma once

/**
 * EOS Testing - P2P (Peer-to-Peer) Manager
 * 
 * Handles direct peer-to-peer communication:
 * - NAT traversal / hole punching
 * - Relay fallback when direct connection fails
 * - Reliable and unreliable message channels
 * - Connection state management
 * 
 * This is the core networking for real-time gameplay in
 * Crab Game-style multiplayer.
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <optional>

#ifndef EOS_STUB_MODE
    #include <eos_sdk.h>
    #include <eos_p2p.h>
#else
    using EOS_ProductUserId = void*;
#endif

namespace eos_testing {

/**
 * P2P packet reliability
 */
enum class PacketReliability {
    UnreliableUnordered,    // Fire and forget (best for position updates)
    ReliableUnordered,      // Guaranteed delivery, any order
    ReliableOrdered         // Guaranteed delivery, in order (best for events)
};

/**
 * Connection status
 */
enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    ConnectionFailed
};

/**
 * P2P connection info
 */
struct PeerConnection {
    EOS_ProductUserId peer_id = nullptr;
    std::string display_name;
    ConnectionStatus status = ConnectionStatus::Disconnected;
    bool is_relay = false;      // True if using relay, false if direct
    uint32_t ping_ms = 0;
    uint64_t bytes_sent = 0;
    uint64_t bytes_received = 0;
};

/**
 * Incoming packet
 */
struct IncomingPacket {
    EOS_ProductUserId sender = nullptr;
    uint8_t channel = 0;
    std::vector<uint8_t> data;
};

/**
 * P2P Configuration
 */
struct P2PConfig {
    // Socket name identifies your game's P2P network
    std::string socket_name = "GameSocket";
    
    // Allow relay connections when direct fails
    bool allow_relay = true;
    
    // Maximum packet size (EOS limit is 1170 bytes)
    uint32_t max_packet_size = 1170;
    
    // Number of channels (0-255)
    // Common setup: 0=unreliable position, 1=reliable events
    uint8_t num_channels = 2;
};

/**
 * Callback types
 */
using ConnectionCallback = std::function<void(EOS_ProductUserId peer, ConnectionStatus status)>;
using PacketCallback = std::function<void(const IncomingPacket& packet)>;

/**
 * P2P Manager
 * 
 * Manages peer-to-peer connections and packet transmission.
 * Uses EOS relay infrastructure for NAT traversal.
 */
class P2PManager {
public:
    static P2PManager& instance();
    
    // Delete copy/move
    P2PManager(const P2PManager&) = delete;
    P2PManager& operator=(const P2PManager&) = delete;
    
    /**
     * Initialize P2P with configuration.
     * Must be called after authentication.
     * 
     * @param config P2P configuration
     * @return true if initialization succeeded
     */
    bool initialize(const P2PConfig& config = {});
    
    /**
     * Shutdown P2P and close all connections.
     */
    void shutdown();
    
    /**
     * Accept incoming connection requests.
     * Call this to allow other peers to connect to you.
     * 
     * @param peer_id Specific peer to accept, or nullptr for all
     */
    void accept_connections(EOS_ProductUserId peer_id = nullptr);
    
    /**
     * Request connection to a peer.
     * Connection is established when both sides accept.
     * 
     * @param peer_id Peer to connect to
     */
    void connect_to_peer(EOS_ProductUserId peer_id);
    
    /**
     * Close connection to a specific peer.
     * 
     * @param peer_id Peer to disconnect from
     */
    void disconnect_from_peer(EOS_ProductUserId peer_id);
    
    /**
     * Close all peer connections.
     */
    void disconnect_all();
    
    /**
     * Send a packet to a specific peer.
     * 
     * @param peer_id Target peer
     * @param data Packet data
     * @param size Data size in bytes
     * @param channel Channel number (default 0)
     * @param reliability Delivery guarantee level
     * @return true if packet was queued for sending
     */
    bool send_packet(EOS_ProductUserId peer_id,
                     const void* data,
                     uint32_t size,
                     uint8_t channel = 0,
                     PacketReliability reliability = PacketReliability::UnreliableUnordered);
    
    /**
     * Send a packet to all connected peers.
     * 
     * @param data Packet data
     * @param size Data size in bytes
     * @param channel Channel number
     * @param reliability Delivery guarantee level
     */
    void broadcast_packet(const void* data,
                          uint32_t size,
                          uint8_t channel = 0,
                          PacketReliability reliability = PacketReliability::UnreliableUnordered);
    
    /**
     * Receive pending packets.
     * Call this regularly (every frame) to process incoming data.
     * 
     * @param max_packets Maximum packets to process per call
     * @return Number of packets processed
     */
    uint32_t receive_packets(uint32_t max_packets = 100);
    
    /**
     * Get connection status for a peer.
     * 
     * @param peer_id Peer to check
     * @return Connection info, or nullopt if not connected
     */
    std::optional<PeerConnection> get_peer_connection(EOS_ProductUserId peer_id) const;
    
    /**
     * Get all peer connections.
     */
    std::vector<PeerConnection> get_all_connections() const;
    
    /**
     * Check if connected to a specific peer.
     */
    bool is_connected_to(EOS_ProductUserId peer_id) const;
    
    /**
     * Get number of connected peers.
     */
    uint32_t get_peer_count() const;
    
    /**
     * Get current configuration.
     */
    const P2PConfig& get_config() const { return m_config; }
    
    // Event callbacks
    ConnectionCallback on_connection_established;
    ConnectionCallback on_connection_closed;
    PacketCallback on_packet_received;

private:
    P2PManager() = default;
    
    void register_callbacks();
    void unregister_callbacks();
    void handle_connection_request(EOS_ProductUserId peer_id);
    void handle_connection_established(EOS_ProductUserId peer_id);
    void handle_connection_closed(EOS_ProductUserId peer_id);
    
    bool m_initialized = false;
    P2PConfig m_config;
    
    std::unordered_map<EOS_ProductUserId, PeerConnection> m_connections;
    mutable std::mutex m_connections_mutex;
    
    // Pending packets queue for thread-safe access
    std::queue<IncomingPacket> m_incoming_packets;
    std::mutex m_packets_mutex;
};

} // namespace eos_testing
