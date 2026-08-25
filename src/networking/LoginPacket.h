#pragma once

#include "Packet.h"

#include <string>

// Credentials submitted by the login screen.
// Field order and types mirror Server/src/network/Packets/LoginPacket.cpp exactly.
//
// SECURITY WARNING: This packet transmits the password in plaintext.
// The connection MUST use TLS/SSL in production to prevent credential theft.
// See SECURITY.md for details.
class LoginPacket final : public Packet
{
public:
    std::string Username;
    std::string Password;

    // Optional tail (J-06): the authenticator code for accounts with 2FA.
    // Written last so it mirrors the server's format exactly; the server
    // tolerates its absence for older clients, and reads empty as "no code".
    std::string TotpCode;

    Opcode getOpcode() const override { return Opcode::Login; }

    const char* getName() const override { return "LoginPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.writeString(Password, ProtocolLimits::MaxPasswordLength);
        buffer.writeString(TotpCode, ProtocolLimits::MaxTotpCodeLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Password = buffer.readString(ProtocolLimits::MaxPasswordLength);
        TotpCode = buffer.readString(ProtocolLimits::MaxTotpCodeLength);
    }
};
