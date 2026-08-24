#pragma once

#include <cstdint>
#include <memory>
#include <string>

class NetworkManager;
class PacketHandler;

/**
 * Authentication service.
 *
 * Screens drive this asynchronously - Begin*(), then poll GetStatus() each
 * frame - because that is the shape a server round trip has.
 *
 * When a NetworkManager session is available the requests go over the wire as
 * LoginPacket / RegisterPacket and resolve on the server's LoginSuccessPacket
 * or LoginFailedPacket. With no server reachable the service falls back to a
 * local mock so the client remains usable offline; the fallback is reported
 * through IsOfflineSession() rather than being disguised as a real login.
 */
class AuthService
{
public:
    enum class Status
    {
        Idle,
        Pending,
        Succeeded,
        Failed
    };

    AuthService();
    virtual ~AuthService();

    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    // Binds the service to a session. Passing nullptr returns it to mock mode.
    void SetNetworkManager(NetworkManager* network);

    // Enables the local mock. Only set from the "offline" config flag; an
    // unreachable server is a failure, not a reason to fake a login.
    void SetOfflineMode(bool offline) { m_OfflineMode = offline; }
    bool IsOfflineMode() const { return m_OfflineMode; }

    // --- Asynchronous API -------------------------------------------------
    // totpCode is the 6-digit authenticator code; empty for accounts without
    // 2FA. When a login fails because the server wants a code, the status
    // message says so and LoginScreen re-submits with the field's contents.
    void BeginLogin(const std::string& usernameOrEmail, const std::string& password,
                    const std::string& totpCode = {});
    void BeginRegister(const std::string& username,
                       const std::string& email,
                       const std::string& password);

    // Advances an in-flight request. Call once per frame.
    void Update(float deltaTime);

    Status GetStatus() const { return m_Status; }
    const std::string& GetStatusMessage() const { return m_StatusMessage; }

    void ResetRequest();

    // True when the last successful login was served by the local mock rather
    // than by the server.
    bool IsOfflineSession() const { return m_OfflineSession; }

    // --- Synchronous API (local validation / mock) ------------------------
    virtual bool Login(const std::string& username, const std::string& password);
    virtual bool Register(const std::string& username,
                          const std::string& password,
                          const std::string& email);

    virtual bool AutoLogin();
    virtual void Logout();

    virtual std::string GetToken() const;
    virtual bool IsAuthenticated() const;
    virtual void ClearCredentials();

    const std::string& GetUsername() const;

    // Whether the account has a world to return to. Drives the Connecting
    // screen's branch between Continue and World Selection.
    bool HasLastWorld() const;

private:
    enum class PendingKind
    {
        None,
        Login,
        Register
    };

    // Installs the LoginSuccess / LoginFailed handlers on the session.
    void AttachHandlers();
    void DetachHandlers();

    // Resolves a request locally when no session is available.
    void ResolveWithMock();

    void Succeed(const std::string& message);
    void Fail(const std::string& message);

    NetworkManager* m_Network = nullptr;

    std::shared_ptr<PacketHandler> m_LoginSuccessHandler;
    std::shared_ptr<PacketHandler> m_LoginFailedHandler;

    Status      m_Status = Status::Idle;
    std::string m_StatusMessage;

    PendingKind m_Pending      = PendingKind::None;
    float       m_PendingTimer = 0.0f;

    std::string m_PendingUsername;
    std::string m_PendingEmail;
    std::string m_PendingPassword;

    bool m_OfflineSession = false;
    bool m_OfflineMode    = false;
};
