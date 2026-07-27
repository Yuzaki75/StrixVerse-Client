#include "KeepAlivePacket.h"
#include "PacketFactory.h"

namespace {
    struct KeepAlivePacketRegistrar {
        KeepAlivePacketRegistrar() {
            PacketFactory::registerPacket(PacketType::KeepAlive, []() {
                return std::make_shared<KeepAlivePacket>();
            });
        }
    };
    static KeepAlivePacketRegistrar registrar;
}