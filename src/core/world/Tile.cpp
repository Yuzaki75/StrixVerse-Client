#include "Tile.h"

namespace StrixVerse
{
    namespace World
    {
        // Tiles are created and destroyed in their tens of thousands during
        // world generation, so they deliberately do no logging: a per-tile log
        // line stalls the client for seconds while the file is written.

        Tile::Tile()
            : m_Type(Type::Grass), m_Solid(true), m_TextureName("grass")
        {
        }

        Tile::Tile(Type type)
            : m_Type(type), m_Solid(true), m_TextureName("grass") // Default, will be set based on type
        {
            // Every type is solid. The chunk stores air as a null tile, so
            // reaching this constructor already means "there is a block here",
            // and the switch exists for the texture name.
            //
            // The flag is still per-type rather than a constant because the
            // types that need to be passable - a ladder, a real liquid - are
            // exactly the ones the server also marks non-solid, and this is
            // where they will differ. Water is the client's stand-in for lava
            // today, which is a block you stand on rather than swim through.
            switch (type)
            {
            case Type::Grass:
                m_TextureName = "grass";
                break;
            case Type::Dirt:
                m_TextureName = "dirt";
                break;
            case Type::Stone:
                m_TextureName = "stone";
                break;
            case Type::Water:
                m_TextureName = "water";
                break;
            case Type::Sand:
                m_TextureName = "sand";
                break;
            default:
                m_TextureName = "grass";
                break;
            }
        }

        Tile::~Tile() = default;

    } // namespace World
} // namespace StrixVerse