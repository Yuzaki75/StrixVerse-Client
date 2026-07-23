#include "RegisterPacket.h"
#include "PacketFactory.h"

// Register this packet type with the factory
namespace {
    struct RegisterPacketRegistrar {
        RegisterPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Register, []() {
                return std::make_shared<RegisterPacket>();
            });
        }
    };
    static RegisterPacketRegistrar registrar;
}
