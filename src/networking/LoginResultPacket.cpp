#include "LoginResultPacket.h"
#include "PacketFactory.h"

namespace {
    struct LoginResultPacketRegistrar {
        LoginResultPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::LoginResult, []() {
                return std::make_shared<LoginResultPacket>();
            });
        }
    };
    static LoginResultPacketRegistrar registrar;
}