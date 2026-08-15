#ifndef PACKET_H
#define PACKET_H

#include <memory>

#include "PacketBuffer.h"
#include "Protocol.h"

// -----------------------------------------------------------------------------
// Packet
//
// Base for every message on the wire. Mirrors the server's Packet interface
// (Server/src/network/Packet/Packet.h) so the two sides stay recognisably the
// same shape: an opcode, a name for logging, and symmetric serialise /
// deserialise over a PacketBuffer.
//
// Serialise/deserialise handle the payload only. The opcode and the length
// prefix are written by Connection when the frame is built.
// -----------------------------------------------------------------------------
class Packet
{
public:
    virtual ~Packet() = default;

    virtual Opcode getOpcode() const = 0;

    // Human-readable name, used in logs and protocol errors.
    virtual const char* getName() const = 0;

    virtual void serialize(PacketBuffer& buffer) const = 0;

    virtual void deserialize(PacketBuffer& buffer) = 0;
};

#endif // PACKET_H
