#pragma once

/**
 * EOS Testing - Voice Chat Manager
 * 
 * Handles real-time voice communication:
 * - Room-based voice chat (lobby/match rooms)
 * - Push-to-talk and open mic modes
 * - Mute/unmute controls
 * - Volume control per player
 * 
 * Essential for party games like Crab Game where
 * players need to communicate during matches.
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

#ifndef EOS_STUB_MODE
    #include <eos_sdk.h>
    #include <eos_rtc.h>
    #include <eos_rtc_audio.h>
#else
    using EOS_ProductUserId = void*;
#endif

namespace eos_testing {

/**
 * Voice input mode
 */
enum class VoiceInputMode {
    OpenMic,        // Always transmitting when sound detected
    PushToTalk      // Only transmit when key held
};

/**
 * Voice participant info
 */
struct VoiceParticipant {
    EOS_ProductUserId user_id = nullptr;
    std::string display_name;
    bool is_speaking = false;
    bool is_muted = false;          // Muted by us locally
    bool is_self_muted = false;     // They muted themselves
    float volume = 1.0f;            // 0.0 to 2.0
};

/**
 * Voice room info
 */
struct VoiceRoom {
    std::string room_name;
    std::vector<VoiceParticipant> participants;
    bool is_connected = false;
};

/**
 * Callback types
 */
using VoiceJoinCallback = std::function<void(bool success, const std::string& room_name)>;
using VoiceLeaveCallback = std::function<void(bool success)>;
using ParticipantCallback = std::function<void(const VoiceParticipant& participant)>;
using SpeakingCallback = std::function<void(EOS_ProductUserId user_id, bool is_speaking)>;

/**
 * Voice Chat Manager
 * 
 * Manages voice chat rooms and audio transmission.
 * Integrates with lobbies for automatic room management.
 */
class VoiceManager {
public:
    static VoiceManager& instance();
    
    // Delete copy/move
    VoiceManager(const VoiceManager&) = delete;
    VoiceManager& operator=(const VoiceManager&) = delete;
    
    /**
     * Initialize voice chat subsystem.
     * Must be called after platform initialization.
     * 
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * Shutdown voice chat.
     */
    void shutdown();
    
    /**
     * Join a voice room.
     * Usually called automatically when joining a lobby.
     * 
     * @param room_name Room identifier (usually lobby ID)
     * @param callback Called when join completes
     */
    void join_room(const std::string& room_name, VoiceJoinCallback callback);
    
    /**
     * Leave the current voice room.
     * 
     * @param callback Called when leave completes
     */
    void leave_room(VoiceLeaveCallback callback);
    
    /**
     * Set voice input mode.
     * 
     * @param mode OpenMic or PushToTalk
     */
    void set_input_mode(VoiceInputMode mode);
    
    /**
     * Set push-to-talk state (only when in PTT mode).
     * 
     * @param talking True when PTT key is held
     */
    void set_push_to_talk(bool talking);
    
    /**
     * Mute/unmute self (stop transmitting).
     * 
     * @param muted Whether to mute self
     */
    void set_self_mute(bool muted);
    
    /**
     * Mute/unmute a specific participant (local only).
     * 
     * @param user_id User to mute
     * @param muted Whether to mute
     */
    void set_participant_mute(EOS_ProductUserId user_id, bool muted);
    
    /**
     * Set volume for a specific participant.
     * 
     * @param user_id User to adjust
     * @param volume Volume level (0.0 = silent, 1.0 = normal, 2.0 = double)
     */
    void set_participant_volume(EOS_ProductUserId user_id, float volume);
    
    /**
     * Set master input volume.
     * 
     * @param volume Volume level (0.0 to 1.0)
     */
    void set_input_volume(float volume);
    
    /**
     * Set master output volume.
     * 
     * @param volume Volume level (0.0 to 1.0)
     */
    void set_output_volume(float volume);
    
    /**
     * Check if currently in a voice room.
     */
    bool is_in_room() const { return m_current_room.has_value(); }
    
    /**
     * Check if self is muted.
     */
    bool is_self_muted() const { return m_self_muted; }
    
    /**
     * Check if currently transmitting.
     */
    bool is_transmitting() const { return m_is_transmitting; }
    
    /**
     * Get current room info.
     */
    std::optional<VoiceRoom> get_current_room() const { return m_current_room; }
    
    /**
     * Get list of participants in current room.
     */
    std::vector<VoiceParticipant> get_participants() const;
    
    // Event callbacks
    ParticipantCallback on_participant_joined;
    ParticipantCallback on_participant_left;
    SpeakingCallback on_speaking_changed;

private:
    VoiceManager() = default;
    
    void register_callbacks();
    void unregister_callbacks();
    
    bool m_initialized = false;
    std::optional<VoiceRoom> m_current_room;
    
    VoiceInputMode m_input_mode = VoiceInputMode::OpenMic;
    bool m_self_muted = false;
    bool m_is_transmitting = false;
    bool m_ptt_active = false;
    
    float m_input_volume = 1.0f;
    float m_output_volume = 1.0f;
};

} // namespace eos_testing
