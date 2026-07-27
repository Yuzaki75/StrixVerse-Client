#include "PlayerRemovePacket.h"
#include "PacketFactory.h"

namespace {
    struct PlayerRemovePacketRegistrar {
        PlayerRemovePacketRegistrar() {
            PacketFactory::registerPacket(PacketType::PlayerRemove, []() {
                return std::make_shared<PlayerRemovePacket>();
            });
        }
    };
    static PlayerRemovePacketRegistrar registrar;
}