#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <functional>
#include <memory>
#include <utility>

#include "Packet.h"

class PacketHandler
{
public:
    virtual ~PacketHandler() = default;

    virtual void handle(const std::shared_ptr<Packet>& packet) = 0;
};

// Adapts a lambda to the PacketHandler interface, so callers that just want to
// react to one opcode do not have to declare a class for it.
class FunctionPacketHandler final : public PacketHandler
{
public:
    using Callback = std::function<void(const std::shared_ptr<Packet>&)>;

    explicit FunctionPacketHandler(Callback callback)
        : m_callback(std::move(callback))
    {
    }

    void handle(const std::shared_ptr<Packet>& packet) override
    {
        if (m_callback)
            m_callback(packet);
    }

private:
    Callback m_callback;
};

#endif // PACKET_HANDLER_H
