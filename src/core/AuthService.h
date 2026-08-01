#pragma once

#include <string>
#include <future>

/**
 * Authentication service abstraction.
 * Handles login, registration, and token management.
 * In a real implementation, this would communicate with a backend.
 */
class AuthService
{
public:
    AuthService() = default;
    virtual ~AuthService() = default;

    // Login with username and password
    // Returns true if login successful, false otherwise.
    // In a real implementation, this would be asynchronous.
    virtual bool Login(const std::string& username, const std::string& password);

    // Register a new account
    // Returns true if registration successful, false otherwise.
    virtual bool Register(const std::string& username, const std::string& password, const std::string& email);

    // Check if we have a saved session (e.g., token) and attempt to auto-login
    // Returns true if auto-login successful.
    virtual bool AutoLogin();

    // Logout and clear saved session
    virtual void Logout();

    // Get the current authentication token (if any)
    virtual std::string GetToken() const;

    // Check if the user is currently authenticated
    virtual bool IsAuthenticated() const;

    // Clear any stored credentials
    virtual void ClearCredentials();
};