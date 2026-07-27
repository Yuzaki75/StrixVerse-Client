#include "SessionPacket.h"
#include "PacketFactory.h"

namespace {
    struct SessionPacketRegistrar {
        SessionPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Session, []() {
                return std::make_shared<SessionPacket>();
            });
        }
    };
    static SessionPacketRegistrar registrar;
}