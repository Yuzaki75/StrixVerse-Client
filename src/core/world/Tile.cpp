#include "Tile.h"

namespace StrixVerse
{
    namespace World
    {
        // Tiles are created and destroyed in their tens of thousands during
        // world generation, so they deliberately do no logging: a per-tile log
        // line stalls the client for seconds while the file is written.

        Tile::Tile()
            : m_Type(Type::Grass), m_Walkable(true), m_TextureName("grass")
        {
        }

        Tile::Tile(Type type)
            : m_Type(type), m_Walkable(true), m_TextureName("grass") // Default, will be set based on type
        {
            // Set walkable and texture based on tile type
            switch (type)
            {
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
        }

        Tile::~Tile() = default;

    } // namespace World
} // namespace StrixVerse