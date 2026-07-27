#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include "Packet.h"

class PacketHandler {
public:
    virtual ~PacketHandler() = default;
    virtual void handle(const std::shared_ptr<Packet>& packet) = 0;
};

#endif // PACKET_HANDLER_H