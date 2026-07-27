#include "PingPacket.h"
#include "PacketFactory.h"

namespace {
    struct PingPacketRegistrar {
        PingPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Ping, []() {
                return std::make_shared<PingPacket>();
            });
        }
    };
    static PingPacketRegistrar registrar;
}