#include "PacketRegistry.h"

#include "ChatMessagePacket.h"
#include "DisconnectPacket.h"
#include "HandshakePacket.h"
#include "KeepAlivePacket.h"
#include "LoginFailedPacket.h"
#include "LoginPacket.h"
#include "LoginSuccessPacket.h"
#include "PingPacket.h"
#include "PlayerMovePacket.h"
#include "PlayerRemovePacket.h"
#include "PlayerSpawnPacket.h"
#include "PongPacket.h"
#include "RegisterPacket.h"
#include "WorldJoinPacket.h"
#include "WorldStatePacket.h"

std::unordered_map<Opcode, PacketRegistry::CreatorFunc>& PacketRegistry::getMap()
{
    static std::unordered_map<Opcode, CreatorFunc> instance;
    return instance;
}

void PacketRegistry::registerPacket(Opcode opcode, CreatorFunc creator)
{
    getMap()[opcode] = std::move(creator);
}

std::shared_ptr<Packet> PacketRegistry::createPacket(Opcode opcode)
{
    const auto it = getMap().find(opcode);
    return it != getMap().end() ? it->second() : nullptr;
}

bool PacketRegistry::isRegistered(Opcode opcode)
{
    return getMap().find(opcode) != getMap().end();
}

void PacketRegistry::clear()
{
    getMap().clear();
}

void PacketRegistry::registerAllPacketTypes()
{
    // Only inbound opcodes strictly need an entry - this table is what turns a
    // received frame into an object - but registering the outbound ones too
    // keeps the client's view of the protocol in one readable place.
    auto add = [](Opcode opcode, CreatorFunc creator)
    {
        registerPacket(opcode, std::move(creator));
    };

    // Connection
    add(Opcode::Handshake,  [] { return std::make_shared<HandshakePacket>(); });
    add(Opcode::Ping,       [] { return std::make_shared<PingPacket>(); });
    add(Opcode::Pong,       [] { return std::make_shared<PongPacket>(); });
    add(Opcode::Disconnect, [] { return std::make_shared<DisconnectPacket>(); });

    // Authentication
    add(Opcode::Login,        [] { return std::make_shared<LoginPacket>(); });
    add(Opcode::LoginSuccess, [] { return std::make_shared<LoginSuccessPacket>(); });
    add(Opcode::LoginFailed,  [] { return std::make_shared<LoginFailedPacket>(); });
    add(Opcode::Register,     [] { return std::make_shared<RegisterPacket>(); });

    // World entry
    add(Opcode::WorldJoin,  [] { return std::make_shared<WorldJoinPacket>(); });
    add(Opcode::WorldState, [] { return std::make_shared<WorldStatePacket>(); });

    // Player replication
    add(Opcode::PlayerSpawn,  [] { return std::make_shared<PlayerSpawnPacket>(); });
    add(Opcode::PlayerMove,   [] { return std::make_shared<PlayerMovePacket>(); });
    add(Opcode::PlayerRemove, [] { return std::make_shared<PlayerRemovePacket>(); });

    // Chat
    add(Opcode::ChatMessage, [] { return std::make_shared<ChatMessagePacket>(); });

    // Keep-alive
    add(Opcode::KeepAlive, [] { return std::make_shared<KeepAlivePacket>(); });
}
