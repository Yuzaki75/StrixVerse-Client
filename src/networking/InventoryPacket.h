#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>
#include <vector>

// Full inventory sync.
//
// Outbound it is a request: the server ignores the body and answers with the
// player's whole inventory. Inbound, InventoryData is an opaque blob the
// server packs itself - see Server/src/entity/Player.cpp, SendFullInventory,
// and the InventoryWire namespace it takes its constants from:
//
//     header, 2 bytes:  [ format : uint8 ][ slotCount : uint8 ]
//     record, 7 bytes:  [ slot : uint8 ]
//                       [ itemId     : uint16 little-endian ]
//                       [ quantity   : uint16 little-endian ]
//                       [ durability : uint16 little-endian ]
//
// This previously described a headerless 5-byte record with itemId first,
// which matched nothing the server ever sent: every field was read from the
// wrong offset, so the client reported phantom occupied slots and garbage
// item ids, and placing a block failed with "doesn't have it".
//
// DecodeSlots() below is the only place that layout is interpreted.
class InventoryPacket final : public Packet
{
public:
    struct Slot
    {
        uint8_t  slot       = 0;
        uint16_t itemId     = 0;
        uint16_t quantity   = 0;
        uint16_t durability = 0;
    };

    // Must match Server/src/network/Packets/InventoryPacket.h, InventoryWire.
    static constexpr uint8_t FormatVersion = 1;
    static constexpr size_t  HeaderSize    = 2;
    static constexpr size_t  BytesPerSlot  = 7;

    uint64_t             PlayerID = 0;
    std::vector<uint8_t> InventoryData;

    Opcode getOpcode() const override { return Opcode::Inventory; }

    const char* getName() const override { return "InventoryPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.write(static_cast<uint32_t>(InventoryData.size()));

        for (uint8_t byte : InventoryData)
            buffer.write(byte);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID = buffer.read<uint64_t>();

        const uint32_t size = buffer.read<uint32_t>();

        // Bound before allocating: the length is attacker-controlled on the
        // wire even when the peer is our own server.
        if (size > ProtocolLimits::MaxPayloadSize)
        {
            InventoryData.clear();
            return;
        }

        InventoryData.resize(size);

        for (uint32_t i = 0; i < size; ++i)
            InventoryData[i] = buffer.read<uint8_t>();
    }

    // Unpacks the blob. Trailing bytes that do not form a whole slot are
    // ignored rather than being read past the end.
    //
    // Only occupied slots are returned. The server transmits every slot,
    // including empty ones, so a stack spent since the last sync clears.
    std::vector<Slot> DecodeSlots() const
    {
        std::vector<Slot> slots;

        // A blob shorter than the header carries nothing at all.
        if (InventoryData.size() < HeaderSize)
            return slots;

        // A format this client does not know cannot be guessed at: the record
        // size could have changed, so every offset below would be wrong.
        const uint8_t format = InventoryData[0];
        if (format != FormatVersion)
            return slots;

        const auto readU16 = [this](size_t at) {
            return static_cast<uint16_t>(InventoryData[at] |
                                         (static_cast<uint16_t>(InventoryData[at + 1]) << 8));
        };

        slots.reserve((InventoryData.size() - HeaderSize) / BytesPerSlot);

        for (size_t i = HeaderSize; i + BytesPerSlot <= InventoryData.size(); i += BytesPerSlot)
        {
            Slot entry;
            entry.slot       = InventoryData[i];
            entry.itemId     = readU16(i + 1);
            entry.quantity   = readU16(i + 3);
            entry.durability = readU16(i + 5);

            // Empty slots are transmitted too; they are not inventory.
            if (entry.itemId == 0 || entry.quantity == 0)
                continue;

            slots.push_back(entry);
        }

        return slots;
    }
};
