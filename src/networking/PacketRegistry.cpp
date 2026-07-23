#include "PacketRegistry.h"
#include <mutex>

std::unordered_map<PacketType, PacketRegistry::CreatorFunc>& PacketRegistry::getMap() {
    static std::unordered_map<PacketType, CreatorFunc> instance;
    return instance;
}

void PacketRegistry::registerPacket(PacketType type, CreatorFunc creator) {
    getMap()[type] = std::move(creator);
}

std::shared_ptr<Packet> PacketRegistry::createPacket(PacketType type) {
    auto it = getMap().find(type);
    if (it != getMap().end()) {
        return it->second();
    }
    return nullptr;
}

void PacketRegistry::clear() {
    getMap().clear();
}
