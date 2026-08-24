#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>

// Client -> server: use what is in this inventory slot.
//
// The server decides entirely what "use" means - it looks the slot up in its
// own copy of the inventory, reads the effect out of the item definition, and
// applies it. ItemID travels only so the server can tell a stale click from a
// current one; nothing about the outcome comes from this packet.
//
// Field order and types mirror Server/src/network/Packets/UseItemPacket.cpp
// exactly.
class UseItemPacket final : public Packet
{
public:
    std::uint64_t PlayerID  = 0;
    std::uint8_t  SlotIndex = 0;
    std::uint16_t ItemID    = 0;
    std::uint16_t Quantity  = 1;

    Opcode getOpcode() const override { return Opcode::UseItem; }

    const char* getName() const override { return "UseItemPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.write(SlotIndex);
        buffer.write(ItemID);
        buffer.write(Quantity);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID  = buffer.read<std::uint64_t>();
        SlotIndex = buffer.read<std::uint8_t>();
        ItemID    = buffer.read<std::uint16_t>();
        Quantity  = buffer.read<std::uint16_t>();
    }
};
