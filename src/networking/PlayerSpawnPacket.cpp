#include "PlayerSpawnPacket.h"
#include "PacketFactory.h"

namespace {
    struct PlayerSpawnPacketRegistrar {
        PlayerSpawnPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::PlayerSpawn, []() {
                return std::make_shared<PlayerSpawnPacket>();
            });
        }
    };
    static PlayerSpawnPacketRegistrar registrar;
}