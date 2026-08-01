#include "AuthService.h"
#include "../core/Logger.h"
#include <chrono>
#include <sstream>
#include <iomanip>

// Simple in-memory storage for demo purposes
static std::string s_StoredToken = "";
static std::string s_StoredUsername = "";
static std::chrono::system_clock::time_point s_TokenExpiry = std::chrono::system_clock::now();

bool AuthService::Login(const std::string& username, const std::string& password)
{
    // TODO: Implement actual network call to login endpoint
    // For now, we'll simulate a successful login for any non-empty credentials
    LOG_INFO("AuthService: Login attempted for user: " + username);
    if (!username.empty() && !password.empty())
    {
        // Simulate generating a token (in reality, this would come from server)
        std::stringstream ss;
        ss << "token_" << username << "_" << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        s_StoredToken = ss.str();
        s_StoredUsername = username;
        s_TokenExpiry = std::chrono::system_clock::now() + std::chrono::hours(1); // 1 hour expiry

        LOG_INFO("AuthService: Login successful for user: " + username);
        return true;
    }
    LOG_WARN("AuthService: Login failed - empty credentials");
    return false;
}

bool AuthService::Register(const std::string& username, const std::string& password, const std::string& email)
{
    // TODO: Implement actual network call to register endpoint
    LOG_INFO("AuthService: Register attempted for user: " + username);
    if (!username.empty() && !password.empty() && !email.empty())
    {
        // Simulate success
        LOG_INFO("AuthService: Registration successful for user: " + username);
        return true;
    }
    LOG_WARN("AuthService: Registration failed - empty fields");
    return false;
}

bool AuthService::AutoLogin()
{
    // Check if we have a valid token
    LOG_INFO("AuthService: AutoLogin attempted");

    if (s_StoredToken.empty())
    {
        LOG_INFO("AuthService: No stored token found");
        return false;
    }

    // Check if token is expired
    auto now = std::chrono::system_clock::now();
    if (now >= s_TokenExpiry)
    {
        LOG_INFO("AuthService: Stored token has expired");
        ClearCredentials();
        return false;
    }

    LOG_INFO("AuthService: AutoLogin successful for user: " + s_StoredUsername);
    return true;
}

void AuthService::Logout()
{
    // TODO: Clear saved token and notify server if needed
    LOG_INFO("AuthService: Logout");
    ClearCredentials();
}

std::string AuthService::GetToken() const
{
    // TODO: Return actual token
    return s_StoredToken;
}

bool AuthService::IsAuthenticated() const
{
    // TODO: Check if token is valid
    if (s_StoredToken.empty())
        return false;

    auto now = std::chrono::system_clock::now();
    if (now >= s_TokenExpiry)
        return false;

    return true;
}

void AuthService::ClearCredentials()
{
    s_StoredToken = "";
    s_StoredUsername = "";
    s_TokenExpiry = std::chrono::system_clock::now();
}