#ifndef PACKET_SENDER_H
#define PACKET_SENDER_H

#include <memory>
#include "Packet.h"

class PacketSender {
public:
    virtual ~PacketSender() = default;
    virtual bool sendPacket(const std::shared_ptr<Packet>& packet) = 0;
};

#endif // PACKET_SENDER_H
