#pragma once

/**
 * EOS Testing - Platform Abstraction Layer
 * 
 * Handles EOS SDK initialization and platform-specific setup.
 * This is the entry point for all EOS functionality.
 */

#include <string>
#include <memory>
#include <functional>

#ifndef EOS_STUB_MODE
    #include <eos_sdk.h>
    #include <eos_init.h>
    #include <eos_logging.h>
#else
    // Stub types when SDK not available
    using EOS_HPlatform = void*;
    using EOS_ProductUserId = void*;
    using EOS_EpicAccountId = void*;
#endif

namespace eos_testing {

/**
 * EOS Platform Configuration
 * Fill these with your Epic Developer Portal credentials
 */
struct PlatformConfig {
    std::string product_name;
    std::string product_version;
    std::string product_id;
    std::string sandbox_id;
    std::string deployment_id;
    std::string client_id;
    std::string client_secret;
    
    // Optional overrides
    std::string cache_directory;
    bool is_server = false;
    uint32_t tick_budget_ms = 0; // 0 = no limit
};

/**
 * Callback types
 */
using InitCallback = std::function<void(bool success, const std::string& message)>;
using TickCallback = std::function<void()>;

/**
 * EOS Platform Manager
 * 
 * Singleton that manages the EOS SDK lifecycle.
 * Must be initialized before any other EOS functionality.
 */
class Platform {
public:
    static Platform& instance();
    
    // Delete copy/move
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    Platform(Platform&&) = delete;
    Platform& operator=(Platform&&) = delete;
    
    /**
     * Initialize the EOS SDK with the provided configuration.
     * This must be called once at application startup.
     * 
     * @param config Platform configuration with credentials
     * @param callback Called when initialization completes
     * @return true if initialization started successfully
     */
    bool initialize(const PlatformConfig& config, InitCallback callback = nullptr);
    
    /**
     * Shutdown the EOS SDK.
     * Call this before application exit.
     */
    void shutdown();
    
    /**
     * Tick the EOS SDK. Must be called regularly (e.g., every frame).
     * @param on_tick Optional callback invoked after SDK tick
     */
    void tick(TickCallback on_tick = nullptr);
    
    /**
     * Check if the platform is initialized and ready.
     */
    bool is_ready() const { return m_initialized; }
    
    /**
     * Get the native EOS platform handle.
     * Returns nullptr if not initialized.
     */
    EOS_HPlatform get_handle() const { return m_platform_handle; }
    
    /**
     * Get the current logged-in user's Product User ID.
     */
    EOS_ProductUserId get_local_user_id() const { return m_local_user_id; }
    
    /**
     * Set the local user after successful authentication.
     */
    void set_local_user_id(EOS_ProductUserId user_id) { m_local_user_id = user_id; }

private:
    Platform() = default;
    ~Platform();
    
    bool m_initialized = false;
    EOS_HPlatform m_platform_handle = nullptr;
    EOS_ProductUserId m_local_user_id = nullptr;
    PlatformConfig m_config;
};

} // namespace eos_testing
