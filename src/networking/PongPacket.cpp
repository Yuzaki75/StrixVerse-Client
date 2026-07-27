#include "PongPacket.h"
#include "PacketFactory.h"

namespace {
    struct PongPacketRegistrar {
        PongPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Pong, []() {
                return std::make_shared<PongPacket>();
            });
        }
    };
    static PongPacketRegistrar registrar;
}