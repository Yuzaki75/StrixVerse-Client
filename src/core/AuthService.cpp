#include "AuthService.h"

#include "Logger.h"
#include "../networking/LoginFailedPacket.h"
#include "../networking/LoginSuccessPacket.h"
#include "../networking/NetworkManager.h"
#include "../networking/PacketHandler.h"

#include <chrono>
#include <format>
#include <sstream>

namespace
{
    // Session state. A real deployment keeps the server's token here; the mock
    // path fabricates one so the rest of the client behaves identically.
    std::string s_StoredToken;
    std::string s_StoredUsername;
    std::chrono::system_clock::time_point s_TokenExpiry = std::chrono::system_clock::now();

    // How long the mock waits before resolving, so the pending state is real.
    constexpr float kMockLatency = 0.9f;

    // How long to wait for the server to answer before giving up.
    constexpr float kServerTimeout = 8.0f;

    constexpr size_t kMinPasswordLength = 8;
    constexpr size_t kMinUsernameLength = 3;

    bool LooksLikeEmail(const std::string& value)
    {
        const size_t at = value.find('@');
        if (at == std::string::npos || at == 0 || at + 1 >= value.size())
            return false;

        return value.find('.', at) != std::string::npos;
    }
}

AuthService::AuthService() = default;

AuthService::~AuthService()
{
    DetachHandlers();
}

void AuthService::SetNetworkManager(NetworkManager* network)
{
    if (m_Network == network)
        return;

    DetachHandlers();
    m_Network = network;

    if (m_Network)
        AttachHandlers();
}

void AuthService::AttachHandlers()
{
    if (!m_Network)
        return;

    m_LoginSuccessHandler = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* success = static_cast<const LoginSuccessPacket*>(packet.get());

            s_StoredUsername = success->Username;
            s_StoredToken    = success->SessionToken;
            s_TokenExpiry    = std::chrono::system_clock::now() + std::chrono::hours(1);

            if (m_Network)
                m_Network->setSession(success->EntityID, success->Username, success->SessionToken);

            m_OfflineSession = false;

            Logger::Info(std::format("AuthService: server authenticated '{}' (player {}).",
                                     success->Username, success->EntityID));

            Succeed(m_Pending == PendingKind::Register ? "Account created" : "Authentication complete");
        });

    m_LoginFailedHandler = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* failure = static_cast<const LoginFailedPacket*>(packet.get());

            Logger::Warning(std::format("AuthService: server rejected the request - {}",
                                        failure->Reason));

            Fail(failure->Reason.empty() ? "The server rejected your credentials."
                                         : failure->Reason);
        });

    m_Network->addPacketHandler(Opcode::LoginSuccess, m_LoginSuccessHandler);
    m_Network->addPacketHandler(Opcode::LoginFailed, m_LoginFailedHandler);
}

void AuthService::DetachHandlers()
{
    if (!m_Network)
        return;

    if (m_LoginSuccessHandler)
        m_Network->removePacketHandler(Opcode::LoginSuccess, m_LoginSuccessHandler);

    if (m_LoginFailedHandler)
        m_Network->removePacketHandler(Opcode::LoginFailed, m_LoginFailedHandler);

    m_LoginSuccessHandler.reset();
    m_LoginFailedHandler.reset();
}

void AuthService::Succeed(const std::string& message)
{
    m_Status        = Status::Succeeded;
    m_StatusMessage = message;
    m_Pending       = PendingKind::None;
    m_PendingPassword.clear();
}

void AuthService::Fail(const std::string& message)
{
    m_Status        = Status::Failed;
    m_StatusMessage = message;
    m_Pending       = PendingKind::None;
    m_PendingPassword.clear();
}

void AuthService::BeginLogin(const std::string& usernameOrEmail, const std::string& password,
                             const std::string& totpCode)
{
    m_Pending         = PendingKind::Login;
    m_PendingUsername = usernameOrEmail;
    m_PendingPassword = password;
    m_PendingEmail.clear();
    m_PendingTimer    = 0.0f;
    m_Status          = Status::Pending;
    m_StatusMessage   = "Authenticating...";

    Logger::Info("AuthService: login requested for user: " + usernameOrEmail);

    if (m_OfflineMode)
        return;   // Resolved locally by Update().

    if (!m_Network || !m_Network->isConnected())
    {
        Fail("Not connected to the server.");
        return;
    }

    // Forwarded rather than stored: per the declaration, a login refused for a
    // missing code is re-submitted by LoginScreen with the field's contents, so
    // there is no retry here that would need to remember it.
    if (!m_Network->sendLogin(usernameOrEmail, password, totpCode))
        Fail("Could not reach the server.");
}

void AuthService::BeginRegister(const std::string& username,
                                const std::string& email,
                                const std::string& password)
{
    m_Pending         = PendingKind::Register;
    m_PendingUsername = username;
    m_PendingEmail    = email;
    m_PendingPassword = password;
    m_PendingTimer    = 0.0f;
    m_Status          = Status::Pending;
    m_StatusMessage   = "Creating account...";

    Logger::Info("AuthService: registration requested for user: " + username);

    if (m_OfflineMode)
        return;   // Resolved locally by Update().

    if (!m_Network || !m_Network->isConnected())
    {
        Fail("Not connected to the server.");
        return;
    }

    if (!m_Network->sendRegister(username, email, password))
        Fail("Could not reach the server.");
}

void AuthService::Update(float deltaTime)
{
    if (m_Status != Status::Pending)
        return;

    m_PendingTimer += deltaTime;

    if (!m_OfflineMode)
    {
        // The handlers above resolve the request; this only catches a server
        // that accepted the frame and then never answered.
        if (m_PendingTimer >= kServerTimeout)
            Fail("The server did not respond.");

        // A session that drops mid-request will never answer.
        if (m_Network && !m_Network->isConnected())
            Fail("Lost connection to the server.");

        return;
    }

    if (m_PendingTimer >= kMockLatency)
        ResolveWithMock();
}

void AuthService::ResolveWithMock()
{
    switch (m_Pending)
    {
    case PendingKind::Login:
        if (Login(m_PendingUsername, m_PendingPassword))
        {
            m_OfflineSession = true;
            Succeed("Signed in offline");
        }
        else
        {
            Fail("Invalid username or password.");
        }
        break;

    case PendingKind::Register:
        if (Register(m_PendingUsername, m_PendingPassword, m_PendingEmail))
        {
            m_OfflineSession = true;
            Succeed("Account created offline");
        }
        else
        {
            Fail("Registration failed. Check your details.");
        }
        break;

    case PendingKind::None:
        m_Status = Status::Idle;
        break;
    }
}

void AuthService::ResetRequest()
{
    m_Pending      = PendingKind::None;
    m_PendingTimer = 0.0f;
    m_Status       = Status::Idle;
    m_StatusMessage.clear();
    m_PendingPassword.clear();
}

bool AuthService::Login(const std::string& username, const std::string& password)
{
    // Local validation, deliberately the same shape the server applies, so the
    // offline path exercises the same failure cases.
    if (username.empty() || password.empty())
    {
        LOG_WARN("AuthService: login failed - empty credentials");
        return false;
    }

    if (password.size() < kMinPasswordLength)
    {
        LOG_WARN("AuthService: login failed - password too short");
        return false;
    }

    std::ostringstream token;
    token << "offline_" << username << "_"
          << std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();

    s_StoredToken    = token.str();
    s_StoredUsername = username;
    s_TokenExpiry    = std::chrono::system_clock::now() + std::chrono::hours(1);

    LOG_INFO("AuthService: offline login accepted for user: " + username);
    return true;
}

bool AuthService::Register(const std::string& username,
                           const std::string& password,
                           const std::string& email)
{
    if (username.size() < kMinUsernameLength)
    {
        LOG_WARN("AuthService: registration failed - username too short");
        return false;
    }

    if (!LooksLikeEmail(email))
    {
        LOG_WARN("AuthService: registration failed - invalid email");
        return false;
    }

    if (password.size() < kMinPasswordLength)
    {
        LOG_WARN("AuthService: registration failed - password too short");
        return false;
    }

    LOG_INFO("AuthService: offline registration accepted for user: " + username);
    return true;
}

bool AuthService::AutoLogin()
{
    if (s_StoredToken.empty())
    {
        LOG_INFO("AuthService: no stored token found");
        return false;
    }

    if (std::chrono::system_clock::now() >= s_TokenExpiry)
    {
        LOG_INFO("AuthService: stored token has expired");
        ClearCredentials();
        return false;
    }

    LOG_INFO("AuthService: auto-login successful for user: " + s_StoredUsername);
    return true;
}

void AuthService::Logout()
{
    LOG_INFO("AuthService: logout");

    if (m_Network)
        m_Network->clearSession();

    ClearCredentials();
}

std::string AuthService::GetToken() const
{
    return s_StoredToken;
}

bool AuthService::IsAuthenticated() const
{
    if (s_StoredToken.empty())
        return false;

    return std::chrono::system_clock::now() < s_TokenExpiry;
}

void AuthService::ClearCredentials()
{
    s_StoredToken.clear();
    s_StoredUsername.clear();
    s_TokenExpiry    = std::chrono::system_clock::now();
    m_OfflineSession = false;
}

const std::string& AuthService::GetUsername() const
{
    return s_StoredUsername;
}

bool AuthService::HasLastWorld() const
{
    // Whether a world can be continued into is a property of the saved session,
    // not of the auth state, and WorldManager answers it per account. This used
    // to guess from process-local "was this account just registered" flags,
    // which did not survive a restart and could not tell two accounts apart.
    return IsAuthenticated();
}
