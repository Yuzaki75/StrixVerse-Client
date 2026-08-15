#ifndef PACKET_DISPATCHER_H
#define PACKET_DISPATCHER_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "Packet.h"
#include "PacketHandler.h"
#include "Protocol.h"

// Routes a received packet to every handler registered for its opcode.
class PacketDispatcher
{
public:
    void addHandler(Opcode opcode, std::shared_ptr<PacketHandler> handler);

    void removeHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler);

    void clear();

    void dispatch(const std::shared_ptr<Packet>& packet);

private:
    std::unordered_map<Opcode, std::vector<std::shared_ptr<PacketHandler>>> m_handlers;
};

#endif // PACKET_DISPATCHER_H
