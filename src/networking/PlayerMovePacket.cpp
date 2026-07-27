#include "PlayerMovePacket.h"
#include "PacketFactory.h"

namespace {
    struct PlayerMovePacketRegistrar {
        PlayerMovePacketRegistrar() {
            PacketFactory::registerPacket(PacketType::PlayerMove, []() {
                return std::make_shared<PlayerMovePacket>();
            });
        }
    };
    static PlayerMovePacketRegistrar registrar;
}