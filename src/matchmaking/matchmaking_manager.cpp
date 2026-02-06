/**
 * EOS Testing - Matchmaking Manager Implementation
 */

#include "eos_testing/matchmaking/matchmaking_manager.hpp"
#include "eos_testing/core/platform.hpp"
#include "eos_testing/auth/auth_manager.hpp"
#include <iostream>

namespace eos_testing {

MatchmakingManager& MatchmakingManager::instance() {
    static MatchmakingManager instance;
    return instance;
}

void MatchmakingManager::start_matchmaking(const MatchmakingCriteria& criteria, 
                                            MatchmakingCallback callback) {
    if (!AuthManager::instance().is_logged_in()) {
        if (callback) callback(false, "Not logged in");
        return;
    }
    
    if (m_status != MatchStatus::Idle) {
        if (callback) callback(false, "Already matchmaking or in session");
        return;
    }
    
    m_current_criteria = criteria;
    m_status = MatchStatus::Searching;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Started matchmaking\n";
    std::cout << "[EOS-STUB] Game mode: " << criteria.game_mode << "\n";
    std::cout << "[EOS-STUB] Players: " << criteria.min_players << "-" << criteria.max_players << "\n";
    
    m_estimated_wait = 15; // Fake 15 second estimate
    
    if (callback) callback(true, "");
    
    // In a real implementation, we'd wait for a match callback
    // For stub, simulate finding a match after user calls tick a few times
#else
    // Real EOS Sessions matchmaking
    if (callback) callback(false, "Not implemented");
#endif
}

void MatchmakingManager::cancel_matchmaking(MatchmakingCallback callback) {
    if (m_status != MatchStatus::Searching) {
        if (callback) callback(true, "");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Matchmaking cancelled\n";
    m_status = MatchStatus::Idle;
    m_estimated_wait = 0;
    if (callback) callback(true, "");
#else
    // Real EOS cancel
    m_status = MatchStatus::Idle;
    if (callback) callback(true, "");
#endif
}

void MatchmakingManager::create_session(const std::string& session_name,
                                         uint32_t max_players,
                                         const std::unordered_map<std::string, std::string>& attributes,
                                         SessionCallback callback) {
    if (!AuthManager::instance().is_logged_in()) {
        if (callback) callback(false, {}, "Not logged in");
        return;
    }
    
    if (m_current_session.has_value()) {
        if (callback) callback(false, {}, "Already in session");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Creating session: " << session_name << "\n";
    
    SessionInfo session;
    session.session_id = "stub-session-" + std::to_string(rand());
    session.session_name = session_name;
    session.max_players = max_players;
    session.current_players = 1;
    session.attributes = attributes;
    session.players.push_back(AuthManager::instance().get_product_user_id());
    
    m_current_session = session;
    m_is_host = true;
    m_status = MatchStatus::InMatch;
    
    std::cout << "[EOS-STUB] Session created: " << session.session_id << "\n";
    
    if (callback) callback(true, session, "");
#else
    // Real EOS Sessions create
    if (callback) callback(false, {}, "Not implemented");
#endif
}

void MatchmakingManager::join_session(const std::string& session_id, SessionCallback callback) {
    if (!AuthManager::instance().is_logged_in()) {
        if (callback) callback(false, {}, "Not logged in");
        return;
    }
    
    if (m_current_session.has_value()) {
        if (callback) callback(false, {}, "Already in session");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Joining session: " << session_id << "\n";
    
    SessionInfo session;
    session.session_id = session_id;
    session.session_name = "Joined Session";
    session.max_players = 8;
    session.current_players = 2;
    session.players.push_back(AuthManager::instance().get_product_user_id());
    
    m_current_session = session;
    m_is_host = false;
    m_status = MatchStatus::InMatch;
    
    std::cout << "[EOS-STUB] Joined session\n";
    
    if (callback) callback(true, session, "");
#else
    // Real EOS Sessions join
    if (callback) callback(false, {}, "Not implemented");
#endif
}

void MatchmakingManager::leave_session(MatchmakingCallback callback) {
    if (!m_current_session.has_value()) {
        if (callback) callback(true, "");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Leaving session: " << m_current_session->session_id << "\n";
    m_current_session.reset();
    m_is_host = false;
    m_status = MatchStatus::Idle;
    if (callback) callback(true, "");
#else
    // Real EOS Sessions leave
    m_current_session.reset();
    m_is_host = false;
    m_status = MatchStatus::Idle;
    if (callback) callback(true, "");
#endif
}

void MatchmakingManager::start_match(MatchmakingCallback callback) {
    if (!m_current_session.has_value() || !m_is_host) {
        if (callback) callback(false, "Not host or not in session");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Starting match!\n";
    
    if (on_match_started) {
        on_match_started();
    }
    
    if (callback) callback(true, "");
#else
    // Lock session and notify players
    if (callback) callback(false, "Not implemented");
#endif
}

void MatchmakingManager::end_match(MatchmakingCallback callback) {
    if (!m_current_session.has_value() || !m_is_host) {
        if (callback) callback(false, "Not host or not in session");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Ending match\n";
    
    if (on_match_ended) {
        on_match_ended();
    }
    
    if (callback) callback(true, "");
#else
    // Unlock session
    if (callback) callback(true, "");
#endif
}

void MatchmakingManager::set_session_attribute(const std::string& key, const std::string& value) {
    if (!m_current_session.has_value() || !m_is_host) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Set session attribute: " << key << " = " << value << "\n";
    m_current_session->attributes[key] = value;
#else
    // Real EOS session attribute update
#endif
}

void MatchmakingManager::register_callbacks() {
#ifndef EOS_STUB_MODE
    // Register for session notifications
#endif
}

void MatchmakingManager::unregister_callbacks() {
#ifndef EOS_STUB_MODE
    // Unregister session notifications
#endif
}

} // namespace eos_testing
