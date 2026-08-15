#include "PacketDispatcher.h"

#include <algorithm>

void PacketDispatcher::addHandler(Opcode opcode, std::shared_ptr<PacketHandler> handler)
{
    if (!handler)
        return;

    m_handlers[opcode].push_back(std::move(handler));
}

void PacketDispatcher::removeHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler)
{
    const auto it = m_handlers.find(opcode);
    if (it == m_handlers.end())
        return;

    auto& handlers = it->second;
    handlers.erase(std::remove(handlers.begin(), handlers.end(), handler), handlers.end());

    if (handlers.empty())
        m_handlers.erase(it);
}

void PacketDispatcher::clear()
{
    m_handlers.clear();
}

void PacketDispatcher::dispatch(const std::shared_ptr<Packet>& packet)
{
    if (!packet)
        return;

    const auto it = m_handlers.find(packet->getOpcode());
    if (it == m_handlers.end())
        return;

    // Iterate over a copy: a handler may register or remove handlers, which
    // would otherwise invalidate the vector mid-loop.
    const auto handlers = it->second;

    for (const auto& handler : handlers)
    {
        if (handler)
            handler->handle(packet);
    }
}
