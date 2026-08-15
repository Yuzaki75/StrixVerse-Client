#ifndef PACKET_REGISTRY_H
#define PACKET_REGISTRY_H

#include <functional>
#include <memory>
#include <unordered_map>

#include "Packet.h"
#include "Protocol.h"

// -----------------------------------------------------------------------------
// PacketRegistry
//
// Maps an opcode arriving on the wire to a factory for the matching packet
// class. Mirrors the server's PacketRegistry so both ends agree on which
// opcodes are legal; an unregistered opcode is dropped rather than guessed at.
// -----------------------------------------------------------------------------
class PacketRegistry
{
public:
    using CreatorFunc = std::function<std::shared_ptr<Packet>()>;

    static void registerPacket(Opcode opcode, CreatorFunc creator);

    // Returns nullptr when the opcode is not registered.
    static std::shared_ptr<Packet> createPacket(Opcode opcode);

    static bool isRegistered(Opcode opcode);

    static void clear();

    // Registers every packet the client understands. Called once during
    // NetworkManager::initialize().
    static void registerAllPacketTypes();

private:
    static std::unordered_map<Opcode, CreatorFunc>& getMap();
};

#endif // PACKET_REGISTRY_H
