/**
 * EOS Testing - Voice Manager Implementation
 */

#include "eos_testing/voice/voice_manager.hpp"
#include "eos_testing/core/platform.hpp"
#include "eos_testing/auth/auth_manager.hpp"
#include <iostream>

namespace eos_testing {

VoiceManager& VoiceManager::instance() {
    static VoiceManager instance;
    return instance;
}

bool VoiceManager::initialize() {
    if (m_initialized) return true;
    
    if (!AuthManager::instance().is_logged_in()) {
        std::cout << "[Voice] Error: Must be logged in before initializing voice\n";
        return false;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Voice chat initialized\n";
    m_initialized = true;
    return true;
#else
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        std::cout << "[Voice] Error: Platform not initialized\n";
        return false;
    }
    
    register_callbacks();
    m_initialized = true;
    std::cout << "[EOS] Voice chat initialized\n";
    return true;
#endif
}

void VoiceManager::shutdown() {
    if (!m_initialized) return;
    
    if (m_current_room.has_value()) {
        leave_room(nullptr);
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Voice chat shutdown\n";
#else
    unregister_callbacks();
#endif
    
    m_initialized = false;
}

void VoiceManager::join_room(const std::string& room_name, VoiceJoinCallback callback) {
    if (!m_initialized) {
        if (callback) callback(false, "");
        return;
    }
    
    if (m_current_room.has_value()) {
        if (callback) callback(false, "Already in a room");
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Joining voice room: " << room_name << "\n";
    
    VoiceRoom room;
    room.room_name = room_name;
    room.is_connected = true;
    
    // Add self as participant
    VoiceParticipant self;
    self.user_id = AuthManager::instance().get_product_user_id();
    self.display_name = AuthManager::instance().get_display_name();
    self.is_speaking = false;
    self.is_muted = m_self_muted;
    room.participants.push_back(self);
    
    m_current_room = room;
    
    std::cout << "[EOS-STUB] Joined voice room\n";
    
    if (callback) callback(true, room_name);
#else
    // Real EOS RTC implementation
    if (callback) callback(false, "Not implemented");
#endif
}

void VoiceManager::leave_room(VoiceLeaveCallback callback) {
    if (!m_current_room.has_value()) {
        if (callback) callback(true);
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Leaving voice room: " << m_current_room->room_name << "\n";
    m_current_room.reset();
    if (callback) callback(true);
#else
    // Real EOS RTC leave
    m_current_room.reset();
    if (callback) callback(true);
#endif
}

void VoiceManager::set_input_mode(VoiceInputMode mode) {
    m_input_mode = mode;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Voice input mode: " 
              << (mode == VoiceInputMode::PushToTalk ? "Push-to-Talk" : "Open Mic") << "\n";
#endif
    
    // Update transmission state based on mode
    if (mode == VoiceInputMode::OpenMic && !m_self_muted) {
        m_is_transmitting = true;
    } else if (mode == VoiceInputMode::PushToTalk) {
        m_is_transmitting = m_ptt_active && !m_self_muted;
    }
}

void VoiceManager::set_push_to_talk(bool talking) {
    m_ptt_active = talking;
    
    if (m_input_mode == VoiceInputMode::PushToTalk) {
        m_is_transmitting = talking && !m_self_muted;
        
#ifdef EOS_STUB_MODE
        std::cout << "[EOS-STUB] PTT: " << (talking ? "TALKING" : "released") << "\n";
#endif
    }
}

void VoiceManager::set_self_mute(bool muted) {
    m_self_muted = muted;
    m_is_transmitting = !muted && (m_input_mode == VoiceInputMode::OpenMic || m_ptt_active);
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Self mute: " << (muted ? "ON" : "OFF") << "\n";
#else
    // Update EOS RTC audio status
#endif
}

void VoiceManager::set_participant_mute(EOS_ProductUserId user_id, bool muted) {
    if (!m_current_room.has_value()) return;
    
    for (auto& participant : m_current_room->participants) {
        if (participant.user_id == user_id) {
            participant.is_muted = muted;
            break;
        }
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Participant mute (" << user_id << "): " 
              << (muted ? "ON" : "OFF") << "\n";
#else
    // Update EOS RTC receive volume to 0 or restore
#endif
}

void VoiceManager::set_participant_volume(EOS_ProductUserId user_id, float volume) {
    if (!m_current_room.has_value()) return;
    
    // Clamp volume
    volume = std::max(0.0f, std::min(2.0f, volume));
    
    for (auto& participant : m_current_room->participants) {
        if (participant.user_id == user_id) {
            participant.volume = volume;
            break;
        }
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Participant volume (" << user_id << "): " << volume << "\n";
#else
    // Update EOS RTC receive volume
#endif
}

void VoiceManager::set_input_volume(float volume) {
    m_input_volume = std::max(0.0f, std::min(1.0f, volume));
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Input volume: " << m_input_volume << "\n";
#else
    // Update EOS RTC input volume
#endif
}

void VoiceManager::set_output_volume(float volume) {
    m_output_volume = std::max(0.0f, std::min(1.0f, volume));
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Output volume: " << m_output_volume << "\n";
#else
    // Update EOS RTC output volume
#endif
}

std::vector<VoiceParticipant> VoiceManager::get_participants() const {
    if (m_current_room.has_value()) {
        return m_current_room->participants;
    }
    return {};
}

void VoiceManager::register_callbacks() {
#ifndef EOS_STUB_MODE
    // Register for RTC notifications
#endif
}

void VoiceManager::unregister_callbacks() {
#ifndef EOS_STUB_MODE
    // Unregister RTC notifications
#endif
}

} // namespace eos_testing
