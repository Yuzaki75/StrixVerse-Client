#include "HandshakePacket.h"
#include "PacketFactory.h"

namespace {
    struct HandshakePacketRegistrar {
        HandshakePacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Handshake, []() {
                return std::make_shared<HandshakePacket>();
            });
        }
    };
    static HandshakePacketRegistrar registrar;
}