/**
 * EOS Testing - Platform Implementation
 */

#include "eos_testing/core/platform.hpp"
#include <iostream>

namespace eos_testing {

Platform& Platform::instance() {
    static Platform instance;
    return instance;
}

Platform::~Platform() {
    if (m_initialized) {
        shutdown();
    }
}

bool Platform::initialize(const PlatformConfig& config, InitCallback callback) {
    if (m_initialized) {
        if (callback) callback(false, "Platform already initialized");
        return false;
    }
    
    m_config = config;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Platform initialized in stub mode\n";
    std::cout << "[EOS-STUB] Product: " << config.product_name << " v" << config.product_version << "\n";
    std::cout << "[EOS-STUB] To use real EOS SDK, download from https://dev.epicgames.com/portal\n";
    m_initialized = true;
    if (callback) callback(true, "Initialized in stub mode");
    return true;
#else
    // Initialize EOS SDK
    EOS_InitializeOptions init_options = {};
    init_options.ApiVersion = EOS_INITIALIZE_API_LATEST;
    init_options.ProductName = config.product_name.c_str();
    init_options.ProductVersion = config.product_version.c_str();
    
    EOS_EResult result = EOS_Initialize(&init_options);
    if (result != EOS_EResult::EOS_Success) {
        std::string error = "EOS_Initialize failed: " + std::to_string(static_cast<int>(result));
        if (callback) callback(false, error);
        return false;
    }
    
    // Set up logging
    EOS_Logging_SetCallback([](const EOS_LogMessage* message) {
        std::cout << "[EOS] " << message->Message << "\n";
    });
    EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Info);
    
    // Create platform
    EOS_Platform_Options platform_options = {};
    platform_options.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    platform_options.ProductId = config.product_id.c_str();
    platform_options.SandboxId = config.sandbox_id.c_str();
    platform_options.DeploymentId = config.deployment_id.c_str();
    platform_options.ClientCredentials.ClientId = config.client_id.c_str();
    platform_options.ClientCredentials.ClientSecret = config.client_secret.c_str();
    platform_options.bIsServer = config.is_server ? EOS_TRUE : EOS_FALSE;
    platform_options.TickBudgetInMilliseconds = config.tick_budget_ms;
    
    if (!config.cache_directory.empty()) {
        platform_options.CacheDirectory = config.cache_directory.c_str();
    }
    
    m_platform_handle = EOS_Platform_Create(&platform_options);
    if (!m_platform_handle) {
        EOS_Shutdown();
        if (callback) callback(false, "EOS_Platform_Create failed");
        return false;
    }
    
    m_initialized = true;
    std::cout << "[EOS] Platform initialized successfully\n";
    if (callback) callback(true, "Platform initialized");
    return true;
#endif
}

void Platform::shutdown() {
    if (!m_initialized) return;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Platform shutdown\n";
#else
    if (m_platform_handle) {
        EOS_Platform_Release(m_platform_handle);
        m_platform_handle = nullptr;
    }
    EOS_Shutdown();
    std::cout << "[EOS] Platform shutdown\n";
#endif
    
    m_initialized = false;
    m_local_user_id = nullptr;
}

void Platform::tick(TickCallback on_tick) {
    if (!m_initialized) return;
    
#ifndef EOS_STUB_MODE
    if (m_platform_handle) {
        EOS_Platform_Tick(m_platform_handle);
    }
#endif
    
    if (on_tick) on_tick();
}

} // namespace eos_testing
