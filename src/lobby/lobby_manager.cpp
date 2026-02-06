/**
 * EOS Testing - Lobby Manager Implementation
 */

#include "eos_testing/lobby/lobby_manager.hpp"
#include "eos_testing/core/platform.hpp"
#include "eos_testing/auth/auth_manager.hpp"
#include <iostream>
#include <algorithm>

namespace eos_testing {

LobbyManager& LobbyManager::instance() {
    static LobbyManager instance;
    return instance;
}

void LobbyManager::create_lobby(const CreateLobbyOptions& options, CreateLobbyCallback callback) {
    if (!AuthManager::instance().is_logged_in()) {
        if (callback) callback(false, "", "Not logged in");
        return;
    }
    
    if (m_current_lobby.has_value()) {
        if (callback) callback(false, "", "Already in a lobby");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Creating lobby: " << options.lobby_name << "\n";
    
    // Create stub lobby
    LobbyInfo lobby;
    lobby.lobby_id = "stub-lobby-" + std::to_string(rand());
    lobby.lobby_name = options.lobby_name;
    lobby.owner_id = AuthManager::instance().get_product_user_id();
    lobby.max_members = options.max_members;
    lobby.current_members = 1;
    lobby.permission = options.permission;
    lobby.allow_join_in_progress = options.allow_join_in_progress;
    lobby.attributes = options.attributes;
    
    // Add self as member
    LobbyMember self_member;
    self_member.user_id = AuthManager::instance().get_product_user_id();
    self_member.display_name = AuthManager::instance().get_display_name();
    self_member.is_owner = true;
    lobby.members.push_back(self_member);
    
    m_current_lobby = lobby;
    
    std::cout << "[EOS-STUB] Lobby created: " << lobby.lobby_id << "\n";
    
    if (callback) callback(true, lobby.lobby_id, "");
#else
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        if (callback) callback(false, "", "Platform not initialized");
        return;
    }
    
    EOS_Lobby_CreateLobbyOptions create_options = {};
    create_options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
    create_options.LocalUserId = AuthManager::instance().get_product_user_id();
    create_options.MaxLobbyMembers = options.max_members;
    create_options.bPresenceEnabled = options.presence_enabled ? EOS_TRUE : EOS_FALSE;
    create_options.bAllowInvites = EOS_TRUE;
    
    switch (options.permission) {
        case LobbyPermission::PublicAdvertised:
            create_options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
            break;
        case LobbyPermission::JoinViaPresence:
            create_options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
            break;
        case LobbyPermission::InviteOnly:
            create_options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
            break;
    }
    
    // Store callback for async completion
    struct CallbackData {
        LobbyManager* manager;
        CreateLobbyCallback callback;
        CreateLobbyOptions options;
    };
    auto* cb_data = new CallbackData{this, callback, options};
    
    EOS_Lobby_CreateLobby(EOS_Platform_GetLobbyInterface(platform), &create_options, cb_data,
        [](const EOS_Lobby_CreateLobbyCallbackInfo* data) {
            auto* cb_data = static_cast<CallbackData*>(data->ClientData);
            
            if (data->ResultCode == EOS_EResult::EOS_Success) {
                std::string lobby_id = data->LobbyId;
                
                // Set up lobby info
                LobbyInfo lobby;
                lobby.lobby_id = lobby_id;
                lobby.lobby_name = cb_data->options.lobby_name;
                lobby.owner_id = AuthManager::instance().get_product_user_id();
                lobby.max_members = cb_data->options.max_members;
                lobby.current_members = 1;
                
                cb_data->manager->m_current_lobby = lobby;
                cb_data->manager->register_callbacks();
                
                if (cb_data->callback) cb_data->callback(true, lobby_id, "");
            } else {
                if (cb_data->callback) cb_data->callback(false, "", "Failed to create lobby");
            }
            
            delete cb_data;
        }
    );
#endif
}

void LobbyManager::join_lobby(const std::string& lobby_id, JoinLobbyCallback callback) {
    if (!AuthManager::instance().is_logged_in()) {
        if (callback) callback(false, {}, "Not logged in");
        return;
    }
    
    if (m_current_lobby.has_value()) {
        if (callback) callback(false, {}, "Already in a lobby");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Joining lobby: " << lobby_id << "\n";
    
    // Create stub joined lobby
    LobbyInfo lobby;
    lobby.lobby_id = lobby_id;
    lobby.lobby_name = "Joined Lobby";
    lobby.max_members = 8;
    lobby.current_members = 2;
    
    LobbyMember self_member;
    self_member.user_id = AuthManager::instance().get_product_user_id();
    self_member.display_name = AuthManager::instance().get_display_name();
    self_member.is_owner = false;
    lobby.members.push_back(self_member);
    
    m_current_lobby = lobby;
    
    std::cout << "[EOS-STUB] Joined lobby successfully\n";
    
    if (callback) callback(true, lobby, "");
#else
    // Real EOS implementation would go here
    if (callback) callback(false, {}, "Not implemented");
#endif
}

void LobbyManager::leave_lobby(LeaveLobbyCallback callback) {
    if (!m_current_lobby.has_value()) {
        if (callback) callback(true);
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Leaving lobby: " << m_current_lobby->lobby_id << "\n";
    m_current_lobby.reset();
    if (callback) callback(true);
#else
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        if (callback) callback(false);
        return;
    }
    
    EOS_Lobby_LeaveLobbyOptions leave_options = {};
    leave_options.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
    leave_options.LocalUserId = AuthManager::instance().get_product_user_id();
    leave_options.LobbyId = m_current_lobby->lobby_id.c_str();
    
    struct CallbackData {
        LobbyManager* manager;
        LeaveLobbyCallback callback;
    };
    auto* cb_data = new CallbackData{this, callback};
    
    EOS_Lobby_LeaveLobby(EOS_Platform_GetLobbyInterface(platform), &leave_options, cb_data,
        [](const EOS_Lobby_LeaveLobbyCallbackInfo* data) {
            auto* cb_data = static_cast<CallbackData*>(data->ClientData);
            
            cb_data->manager->unregister_callbacks();
            cb_data->manager->m_current_lobby.reset();
            
            if (cb_data->callback) {
                cb_data->callback(data->ResultCode == EOS_EResult::EOS_Success);
            }
            
            delete cb_data;
        }
    );
#endif
}

void LobbyManager::search_lobbies(uint32_t max_results,
                                   const std::unordered_map<std::string, std::string>& filters,
                                   SearchLobbyCallback callback) {
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Searching for lobbies (max " << max_results << ")\n";
    
    // Return some fake results
    std::vector<LobbySearchResult> results;
    
    LobbySearchResult result1;
    result1.lobby_id = "stub-lobby-001";
    result1.lobby_name = "Fun Game Room";
    result1.current_members = 3;
    result1.max_members = 8;
    results.push_back(result1);
    
    LobbySearchResult result2;
    result2.lobby_id = "stub-lobby-002";
    result2.lobby_name = "Competitive Match";
    result2.current_members = 6;
    result2.max_members = 8;
    results.push_back(result2);
    
    std::cout << "[EOS-STUB] Found " << results.size() << " lobbies\n";
    
    if (callback) callback(true, results);
#else
    // Real EOS implementation
    if (callback) callback(false, {});
#endif
}

void LobbyManager::set_lobby_attribute(const std::string& key, const std::string& value) {
    if (!m_current_lobby.has_value() || !is_owner()) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Set lobby attribute: " << key << " = " << value << "\n";
    m_current_lobby->attributes[key] = value;
    if (on_lobby_updated) on_lobby_updated(*m_current_lobby);
#else
    // Real EOS implementation
#endif
}

void LobbyManager::set_member_attribute(const std::string& key, const std::string& value) {
    if (!m_current_lobby.has_value()) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Set member attribute: " << key << " = " << value << "\n";
    auto user_id = AuthManager::instance().get_product_user_id();
    for (auto& member : m_current_lobby->members) {
        if (member.user_id == user_id) {
            member.attributes[key] = value;
            break;
        }
    }
    if (on_lobby_updated) on_lobby_updated(*m_current_lobby);
#else
    // Real EOS implementation
#endif
}

void LobbyManager::set_ready(bool ready) {
    set_member_attribute("ready", ready ? "true" : "false");
    
    if (m_current_lobby.has_value()) {
        auto user_id = AuthManager::instance().get_product_user_id();
        for (auto& member : m_current_lobby->members) {
            if (member.user_id == user_id) {
                member.is_ready = ready;
                break;
            }
        }
    }
}

void LobbyManager::kick_member(EOS_ProductUserId user_id) {
    if (!m_current_lobby.has_value() || !is_owner()) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Kicked member: " << user_id << "\n";
    auto& members = m_current_lobby->members;
    members.erase(std::remove_if(members.begin(), members.end(),
        [user_id](const LobbyMember& m) { return m.user_id == user_id; }), members.end());
    m_current_lobby->current_members = static_cast<uint32_t>(members.size());
    if (on_member_leave) on_member_leave(m_current_lobby->lobby_id, user_id);
#else
    // Real EOS implementation
#endif
}

void LobbyManager::promote_member(EOS_ProductUserId user_id) {
    if (!m_current_lobby.has_value() || !is_owner()) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Promoted member: " << user_id << "\n";
    for (auto& member : m_current_lobby->members) {
        member.is_owner = (member.user_id == user_id);
    }
    m_current_lobby->owner_id = user_id;
    if (on_lobby_updated) on_lobby_updated(*m_current_lobby);
#else
    // Real EOS implementation
#endif
}

void LobbyManager::send_chat_message(const std::string& message) {
    if (!m_current_lobby.has_value()) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Chat: " << AuthManager::instance().get_display_name() 
              << ": " << message << "\n";
    if (on_chat_message) {
        on_chat_message(AuthManager::instance().get_display_name(), message);
    }
#else
    // Real EOS doesn't have built-in chat - would use P2P or custom solution
#endif
}

bool LobbyManager::is_owner() const {
    if (!m_current_lobby.has_value()) return false;
    return m_current_lobby->owner_id == AuthManager::instance().get_product_user_id();
}

bool LobbyManager::all_members_ready() const {
    if (!m_current_lobby.has_value()) return false;
    
    for (const auto& member : m_current_lobby->members) {
        if (!member.is_ready && !member.is_owner) {
            return false;
        }
    }
    return true;
}

void LobbyManager::register_callbacks() {
    if (m_callbacks_registered) return;
    
#ifndef EOS_STUB_MODE
    // Register for lobby notifications
#endif
    
    m_callbacks_registered = true;
}

void LobbyManager::unregister_callbacks() {
    if (!m_callbacks_registered) return;
    
#ifndef EOS_STUB_MODE
    // Unregister lobby notifications
#endif
    
    m_callbacks_registered = false;
}

void LobbyManager::refresh_lobby_info() {
#ifndef EOS_STUB_MODE
    // Refresh lobby data from EOS
#endif
}

} // namespace eos_testing
