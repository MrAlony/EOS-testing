/**
 * EOS Testing - Authentication Manager Implementation
 */

#include "eos_testing/auth/auth_manager.hpp"
#include "eos_testing/core/platform.hpp"
#include <iostream>

namespace eos_testing {

AuthManager& AuthManager::instance() {
    static AuthManager instance;
    return instance;
}

void AuthManager::login_device_id(const std::string& display_name, AuthCallback callback) {
    if (m_logged_in) {
        AuthResult result;
        result.success = false;
        result.error_message = "Already logged in";
        if (callback) callback(result);
        return;
    }
    
    m_pending_callback = callback;
    m_display_name = display_name;
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Device ID login for: " << display_name << "\n";
    
    // Simulate successful login
    m_logged_in = true;
    m_product_user_id = reinterpret_cast<EOS_ProductUserId>(0x12345678);
    
    Platform::instance().set_local_user_id(m_product_user_id);
    
    AuthResult result;
    result.success = true;
    result.display_name = display_name;
    result.product_user_id = m_product_user_id;
    
    std::cout << "[EOS-STUB] Login successful! User ID: " << m_product_user_id << "\n";
    
    if (callback) callback(result);
#else
    // Create device ID first, then connect
    create_device_id(display_name, callback);
#endif
}

void AuthManager::login_developer(const std::string& host, 
                                   const std::string& credential_name,
                                   AuthCallback callback) {
    if (m_logged_in) {
        AuthResult result;
        result.success = false;
        result.error_message = "Already logged in";
        if (callback) callback(result);
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Developer login via " << host << " as " << credential_name << "\n";
    
    m_logged_in = true;
    m_display_name = credential_name;
    m_product_user_id = reinterpret_cast<EOS_ProductUserId>(0x87654321);
    
    Platform::instance().set_local_user_id(m_product_user_id);
    
    AuthResult result;
    result.success = true;
    result.display_name = credential_name;
    result.product_user_id = m_product_user_id;
    
    if (callback) callback(result);
#else
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        AuthResult result;
        result.success = false;
        result.error_message = "Platform not initialized";
        if (callback) callback(result);
        return;
    }
    
    // Auth with Developer Tool
    EOS_Auth_Credentials credentials = {};
    credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
    credentials.Id = host.c_str();
    credentials.Token = credential_name.c_str();
    
    EOS_Auth_LoginOptions login_options = {};
    login_options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    login_options.Credentials = &credentials;
    
    m_pending_callback = callback;
    m_display_name = credential_name;
    
    EOS_Auth_Login(EOS_Platform_GetAuthInterface(platform), &login_options, this,
        [](const EOS_Auth_LoginCallbackInfo* data) {
            auto* self = static_cast<AuthManager*>(data->ClientData);
            if (data->ResultCode == EOS_EResult::EOS_Success) {
                self->m_epic_account_id = data->LocalUserId;
                self->connect_login(self->m_pending_callback);
            } else {
                AuthResult result;
                result.success = false;
                result.error_message = "Developer auth failed";
                if (self->m_pending_callback) self->m_pending_callback(result);
            }
        }
    );
#endif
}

void AuthManager::login_epic_account(AuthCallback callback) {
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Epic Account login not available in stub mode\n";
    AuthResult result;
    result.success = false;
    result.error_message = "Epic Account login requires real SDK";
    if (callback) callback(result);
#else
    // This would open browser for Epic account login
    // Implementation depends on platform
    AuthResult result;
    result.success = false;
    result.error_message = "Epic Account login not implemented - use DeviceID or Developer auth";
    if (callback) callback(result);
#endif
}

void AuthManager::logout(LogoutCallback callback) {
    if (!m_logged_in) {
        if (callback) callback(true);
        return;
    }
    
#ifdef EOS_STUB_MODE
    std::cout << "[EOS-STUB] Logout\n";
    m_logged_in = false;
    m_display_name.clear();
    m_product_user_id = nullptr;
    m_epic_account_id = nullptr;
    if (callback) callback(true);
#else
    // Actual logout logic
    m_logged_in = false;
    m_display_name.clear();
    m_product_user_id = nullptr;
    m_epic_account_id = nullptr;
    if (callback) callback(true);
#endif
}

void AuthManager::connect_login(AuthCallback callback) {
#ifndef EOS_STUB_MODE
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        AuthResult result;
        result.success = false;
        result.error_message = "Platform not initialized";
        if (callback) callback(result);
        return;
    }
    
    // Get auth token from logged in Epic account
    EOS_Auth_CopyUserAuthTokenOptions token_options = {};
    token_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
    
    EOS_Auth_Token* auth_token = nullptr;
    EOS_Auth_CopyUserAuthToken(EOS_Platform_GetAuthInterface(platform), 
                                &token_options, m_epic_account_id, &auth_token);
    
    if (!auth_token) {
        AuthResult result;
        result.success = false;
        result.error_message = "Failed to get auth token";
        if (callback) callback(result);
        return;
    }
    
    // Connect with the auth token
    EOS_Connect_Credentials connect_credentials = {};
    connect_credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    connect_credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
    connect_credentials.Token = auth_token->AccessToken;
    
    EOS_Connect_LoginOptions connect_options = {};
    connect_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    connect_options.Credentials = &connect_credentials;
    
    m_pending_callback = callback;
    
    EOS_Connect_Login(EOS_Platform_GetConnectInterface(platform), &connect_options, this,
        [](const EOS_Connect_LoginCallbackInfo* data) {
            auto* self = static_cast<AuthManager*>(data->ClientData);
            
            AuthResult result;
            if (data->ResultCode == EOS_EResult::EOS_Success) {
                self->m_product_user_id = data->LocalUserId;
                self->m_logged_in = true;
                Platform::instance().set_local_user_id(self->m_product_user_id);
                
                result.success = true;
                result.display_name = self->m_display_name;
                result.product_user_id = self->m_product_user_id;
                result.epic_account_id = self->m_epic_account_id;
            } else {
                result.success = false;
                result.error_message = "Connect login failed";
            }
            
            if (self->m_pending_callback) self->m_pending_callback(result);
        }
    );
    
    EOS_Auth_Token_Release(auth_token);
#endif
}

void AuthManager::create_device_id(const std::string& display_name, AuthCallback callback) {
#ifndef EOS_STUB_MODE
    auto platform = Platform::instance().get_handle();
    if (!platform) {
        AuthResult result;
        result.success = false;
        result.error_message = "Platform not initialized";
        if (callback) callback(result);
        return;
    }
    
    // Create device ID
    EOS_Connect_CreateDeviceIdOptions create_options = {};
    create_options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
    create_options.DeviceModel = "PC";
    
    m_pending_callback = callback;
    m_display_name = display_name;
    
    EOS_Connect_CreateDeviceId(EOS_Platform_GetConnectInterface(platform), &create_options, this,
        [](const EOS_Connect_CreateDeviceIdCallbackInfo* data) {
            auto* self = static_cast<AuthManager*>(data->ClientData);
            
            // Device ID creation might fail if already exists - that's OK
            if (data->ResultCode == EOS_EResult::EOS_Success || 
                data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed) {
                
                // Now login with device ID
                auto platform = Platform::instance().get_handle();
                
                EOS_Connect_Credentials credentials = {};
                credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
                credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
                credentials.Token = nullptr;
                
                EOS_Connect_UserLoginInfo user_info = {};
                user_info.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
                user_info.DisplayName = self->m_display_name.c_str();
                
                EOS_Connect_LoginOptions login_options = {};
                login_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
                login_options.Credentials = &credentials;
                login_options.UserLoginInfo = &user_info;
                
                EOS_Connect_Login(EOS_Platform_GetConnectInterface(platform), &login_options, self,
                    [](const EOS_Connect_LoginCallbackInfo* data) {
                        auto* self = static_cast<AuthManager*>(data->ClientData);
                        
                        AuthResult result;
                        if (data->ResultCode == EOS_EResult::EOS_Success) {
                            self->m_product_user_id = data->LocalUserId;
                            self->m_logged_in = true;
                            Platform::instance().set_local_user_id(self->m_product_user_id);
                            
                            result.success = true;
                            result.display_name = self->m_display_name;
                            result.product_user_id = self->m_product_user_id;
                            
                            std::cout << "[EOS] Device ID login successful\n";
                        } else if (data->ResultCode == EOS_EResult::EOS_InvalidUser) {
                            // Need to create a new user
                            // This requires calling EOS_Connect_CreateUser
                            result.success = false;
                            result.error_message = "New user - needs account linking";
                        } else {
                            result.success = false;
                            result.error_message = "Connect login failed";
                        }
                        
                        if (self->m_pending_callback) self->m_pending_callback(result);
                    }
                );
            } else {
                AuthResult result;
                result.success = false;
                result.error_message = "Failed to create device ID";
                if (self->m_pending_callback) self->m_pending_callback(result);
            }
        }
    );
#endif
}

} // namespace eos_testing
