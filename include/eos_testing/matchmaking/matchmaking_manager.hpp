#pragma once

/**
 * EOS Testing - Matchmaking Manager
 * 
 * Handles skill-based and criteria-based matchmaking:
 * - Creating matchmaking tickets
 * - Searching for matches
 * - Match found notifications
 * - Session management
 * 
 * For Crab Game-style games, this enables quick-play
 * where players are automatically matched into games.
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

#ifndef EOS_STUB_MODE
    #include <eos_sdk.h>
    #include <eos_sessions.h>
#else
    using EOS_ProductUserId = void*;
#endif

namespace eos_testing {

/**
 * Match status
 */
enum class MatchStatus {
    Idle,               // Not searching
    Searching,          // Looking for match
    MatchFound,         // Match found, waiting to join
    Joining,            // Joining the matched session
    InMatch             // Currently in match
};

/**
 * Session info (a "match" in progress)
 */
struct SessionInfo {
    std::string session_id;
    std::string session_name;
    std::string host_address;       // For dedicated server mode
    uint32_t max_players = 0;
    uint32_t current_players = 0;
    
    // Custom session attributes
    std::unordered_map<std::string, std::string> attributes;
    
    // Players in session
    std::vector<EOS_ProductUserId> players;
};

/**
 * Matchmaking criteria
 */
struct MatchmakingCriteria {
    // Game mode filter
    std::string game_mode;
    
    // Region preference (empty = any)
    std::string preferred_region;
    
    // Skill range (0 = no skill matching)
    uint32_t min_skill = 0;
    uint32_t max_skill = 0;
    
    // Player count requirements
    uint32_t min_players = 2;
    uint32_t max_players = 8;
    
    // Additional filters
    std::unordered_map<std::string, std::string> custom_filters;
    
    // Timeout in seconds (0 = no timeout)
    uint32_t timeout_seconds = 60;
};

/**
 * Callback types
 */
using MatchFoundCallback = std::function<void(const SessionInfo& session)>;
using MatchmakingCallback = std::function<void(bool success, const std::string& error)>;
using SessionCallback = std::function<void(bool success, const SessionInfo& session, const std::string& error)>;

/**
 * Matchmaking Manager
 * 
 * Handles automatic matchmaking into game sessions.
 * Works alongside lobbies for pre-game gathering.
 */
class MatchmakingManager {
public:
    static MatchmakingManager& instance();
    
    // Delete copy/move
    MatchmakingManager(const MatchmakingManager&) = delete;
    MatchmakingManager& operator=(const MatchmakingManager&) = delete;
    
    /**
     * Start searching for a match.
     * 
     * @param criteria Matchmaking criteria
     * @param callback Called when search starts/fails
     */
    void start_matchmaking(const MatchmakingCriteria& criteria, MatchmakingCallback callback);
    
    /**
     * Cancel matchmaking search.
     * 
     * @param callback Called when cancelled
     */
    void cancel_matchmaking(MatchmakingCallback callback);
    
    /**
     * Create a new game session (host mode).
     * Other players can join via matchmaking or direct join.
     * 
     * @param session_name Display name for the session
     * @param max_players Maximum player count
     * @param attributes Custom session attributes
     * @param callback Called when session is created
     */
    void create_session(const std::string& session_name,
                        uint32_t max_players,
                        const std::unordered_map<std::string, std::string>& attributes,
                        SessionCallback callback);
    
    /**
     * Join a specific session by ID.
     * 
     * @param session_id Session to join
     * @param callback Called when join completes
     */
    void join_session(const std::string& session_id, SessionCallback callback);
    
    /**
     * Leave the current session.
     * 
     * @param callback Called when leave completes
     */
    void leave_session(MatchmakingCallback callback);
    
    /**
     * Start the match (host only).
     * Locks the session and notifies all players.
     * 
     * @param callback Called when match starts
     */
    void start_match(MatchmakingCallback callback);
    
    /**
     * End the current match (host only).
     * 
     * @param callback Called when match ends
     */
    void end_match(MatchmakingCallback callback);
    
    /**
     * Update session attribute (host only).
     * 
     * @param key Attribute key
     * @param value Attribute value
     */
    void set_session_attribute(const std::string& key, const std::string& value);
    
    /**
     * Get current match status.
     */
    MatchStatus get_status() const { return m_status; }
    
    /**
     * Check if currently in a session.
     */
    bool is_in_session() const { return m_current_session.has_value(); }
    
    /**
     * Check if we're the session host.
     */
    bool is_host() const { return m_is_host; }
    
    /**
     * Get current session info.
     */
    std::optional<SessionInfo> get_current_session() const { return m_current_session; }
    
    /**
     * Get estimated wait time in seconds.
     */
    uint32_t get_estimated_wait_time() const { return m_estimated_wait; }
    
    // Event callbacks
    MatchFoundCallback on_match_found;
    std::function<void(EOS_ProductUserId player)> on_player_joined;
    std::function<void(EOS_ProductUserId player)> on_player_left;
    std::function<void()> on_match_started;
    std::function<void()> on_match_ended;
    std::function<void(const std::string& reason)> on_matchmaking_failed;

private:
    MatchmakingManager() = default;
    
    void register_callbacks();
    void unregister_callbacks();
    
    MatchStatus m_status = MatchStatus::Idle;
    std::optional<SessionInfo> m_current_session;
    bool m_is_host = false;
    uint32_t m_estimated_wait = 0;
    
    MatchmakingCriteria m_current_criteria;
};

} // namespace eos_testing
