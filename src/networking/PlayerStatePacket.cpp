#include "PlayerStatePacket.h"
#include "PacketFactory.h"

namespace {
    struct PlayerStatePacketRegistrar {
        PlayerStatePacketRegistrar() {
            PacketFactory::registerPacket(PacketType::PlayerState, []() {
                return std::make_shared<PlayerStatePacket>();
            });
        }
    };
    static PlayerStatePacketRegistrar registrar;
}