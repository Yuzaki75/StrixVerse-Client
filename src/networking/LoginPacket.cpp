#include "LoginPacket.h"
#include "PacketFactory.h"

namespace {
    struct LoginPacketRegistrar {
        LoginPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Login, []() {
                return std::make_shared<LoginPacket>();
            });
        }
    };
    static LoginPacketRegistrar registrar;
}