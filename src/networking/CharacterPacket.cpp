#include "CharacterPacket.h"
#include "PacketFactory.h"

namespace {
    struct CharacterPacketRegistrar {
        CharacterPacketRegistrar() {
            PacketFactory::registerPacket(PacketType::Character, []() {
                return std::make_shared<CharacterPacket>();
            });
        }
    };
    static CharacterPacketRegistrar registrar;
}