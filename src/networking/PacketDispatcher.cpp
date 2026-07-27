#include "PacketDispatcher.h"

void PacketDispatcher::addHandler(PacketType type, std::shared_ptr<PacketHandler> handler) {
    m_handlers[type].push_back(handler);
}

void PacketDispatcher::removeHandler(PacketType type, std::shared_ptr<PacketHandler> handler) {
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), handler), vec.end());
        if (vec.empty()) {
            m_handlers.erase(it);
        }
    }
}

void PacketDispatcher::dispatch(const std::shared_ptr<Packet>& packet) {
    auto it = m_handlers.find(packet->getType());
    if (it != m_handlers.end()) {
        for (auto& handler : it->second) {
            handler->handle(packet);
        }
    }
}