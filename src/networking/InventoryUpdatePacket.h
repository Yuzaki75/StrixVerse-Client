#pragma once

#include "Packet.h"

// A single inventory slot changing.
//
// SERVER -> CLIENT ONLY. The server refuses this opcode inbound and drops the
// connection for a protocol violation, because accepting a client-authored
// slot would let any client mint items. See Server::SetupPacketHandlers.
// Nothing here should ever call sendPacket with one of these.
//
// Field order mirrors Server/src/network/Packets/InventoryUpdatePacket.cpp.
class InventoryUpdatePacket final : public Packet
{
public:
    uint64_t PlayerID   = 0;
    uint8_t  SlotIndex  = 0;
    uint16_t ItemID     = 0;
    uint16_t Quantity   = 0;
    uint16_t Durability = 0;

    Opcode getOpcode() const override { return Opcode::InventoryUpdate; }

    const char* getName() const override { return "InventoryUpdatePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.write(SlotIndex);
        buffer.write(ItemID);
        buffer.write(Quantity);
        buffer.write(Durability);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID   = buffer.read<uint64_t>();
        SlotIndex  = buffer.read<uint8_t>();
        ItemID     = buffer.read<uint16_t>();
        Quantity   = buffer.read<uint16_t>();
        Durability = buffer.read<uint16_t>();
    }
};
