#include "Tile.h"
#include "../Logger.h"

namespace StrixVerse {
    namespace World {

        Tile::Tile()
            : m_Type(Type::Grass)
            , m_Walkable(true)
            , m_TextureName("grass")
        {
            LOG_DEBUG("Tile: Created default grass tile");
        }

        Tile::Tile(Type type)
            : m_Type(type)
            , m_Walkable(true)
            , m_TextureName("grass") // Default, will be set based on type
        {
            // Set walkable and texture based on tile type
            switch (type) {
                case Type::Grass:
                    m_Walkable = true;
                    m_TextureName = "grass";
                    break;
                case Type::Dirt:
                    m_Walkable = true;
                    m_TextureName = "dirt";
                    break;
                case Type::Stone:
                    m_Walkable = true;
                    m_TextureName = "stone";
                    break;
                case Type::Water:
                    m_Walkable = false;
                    m_TextureName = "water";
                    break;
                case Type::Sand:
                    m_Walkable = true;
                    m_TextureName = "sand";
                    break;
                default:
                    m_Walkable = true;
                    m_TextureName = "grass";
                    break;
            }
            LOG_DEBUG("Tile: Created tile of type {}", static_cast<int>(type));
        }

        Tile::~Tile() {
            LOG_DEBUG("Tile: Destroyed tile");
        }

    } // namespace World
} // namespace StrixVerse