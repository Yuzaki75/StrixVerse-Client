#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <string>
#include <memory>
#include "PacketBuffer.h"

enum class PacketType : uint8_t {
    // Authentication
    Handshake = 1,
    Login,
    Register,
    LoginResult,
    Session,
    Character,
    // World
    PlayerSpawn,
    PlayerMove,
    PlayerRemove,
    PlayerState,
    Chunk,
    TileUpdate,
    BlockBreak,
    BlockPlace,
    // Inventory
    Inventory,
    ItemPickup,
    ItemDrop,
    ItemMove,
    Equipment,
    // Chat
    Chat,
    Whisper,
    SystemMessage,
    // Gameplay
    Interact,
    Door,
    Portal,
    NPC,
    Trade,
    // KeepAlive
    Ping,
    Pong,
    KeepAlive
};

class Packet {
public:
    virtual ~Packet() = default;
    
    // Get the packet type
    virtual PacketType getType() const = 0;
    
    // Serialize the packet's data (excluding type) into the buffer
    virtual void serialize(PacketBuffer& buffer) const = 0;
    
    // Deserialize the packet's data (excluding type) from the buffer
    virtual void deserialize(PacketBuffer& buffer) = 0;
    
    // Convenience methods
    std::shared_ptr<Packet> clone() const {
        // This is a simple implementation; derived classes can override if needed
        auto cloned = createInstance();
        // Copy data? This is tricky without RTTI. We'll rely on derived classes to implement properly.
        // For now, we'll leave it as a placeholder; each packet can implement its own clone if needed.
        return cloned;
    }

protected:
    // Factory method to create a new instance of the derived type
    // Derived classes must override this to return a new instance of themselves.
    virtual std::shared_ptr<Packet> createInstance() const = 0;
};

#endif // PACKET_H
