#include "UIPatterns.h"

#include "../core/AssetManager.h"
#include "../graphics/Texture.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    constexpr const char* kPixelGridKey = "procedural://ui-pixel-grid";
    constexpr const char* kGrainKey     = "procedural://ui-grain";

    constexpr unsigned int kPixelGridSize = 64;
    constexpr unsigned int kGrainSize     = 256;

    // Radius of one lattice dot, in texels of the tile above. The CSS uses a
    // 1px dot on a 24px grid, scaled onto the design canvas.
    constexpr float kDotRadius = 1.0f * (1080.0f / 460.0f) *
                                 (static_cast<float>(kPixelGridSize) / UIPatterns::kPixelGridTileSize);
}

std::shared_ptr<Texture> UIPatterns::GetPixelGrid(AssetManager& assets)
{
    if (auto cached = assets.GetTexture(kPixelGridKey))
        return cached;

    std::vector<unsigned char> pixels(static_cast<size_t>(kPixelGridSize) * kPixelGridSize * 4, 0);

    const float centre = static_cast<float>(kPixelGridSize) * 0.5f;

    for (unsigned int y = 0; y < kPixelGridSize; ++y)
    {
        for (unsigned int x = 0; x < kPixelGridSize; ++x)
        {
            const float dx = static_cast<float>(x) + 0.5f - centre;
            const float dy = static_cast<float>(y) + 0.5f - centre;
            const float distance = std::sqrt(dx * dx + dy * dy);

            // One texel of feathering keeps the dot from aliasing when the
            // window scale is not 1:1.
            const float coverage = std::clamp(kDotRadius + 0.5f - distance, 0.0f, 1.0f);

            const size_t index = (static_cast<size_t>(y) * kPixelGridSize + x) * 4;

            // White RGB with coverage in alpha: the tint supplies the colour.
            pixels[index + 0] = 255;
            pixels[index + 1] = 255;
            pixels[index + 2] = 255;
            pixels[index + 3] = static_cast<unsigned char>(coverage * 255.0f);
        }
    }

    return assets.CreateTexture(kPixelGridKey, kPixelGridSize, kPixelGridSize, pixels.data(), 4);
}

std::shared_ptr<Texture> UIPatterns::GetGrain(AssetManager& assets)
{
    if (auto cached = assets.GetTexture(kGrainKey))
        return cached;

    std::vector<unsigned char> pixels(static_cast<size_t>(kGrainSize) * kGrainSize * 4);

    // xorshift32 with a fixed seed: identical grain on every run.
    uint32_t state = 0x9E3779B9u;

    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;

        pixels[i + 0] = 255;
        pixels[i + 1] = 255;
        pixels[i + 2] = 255;
        pixels[i + 3] = static_cast<unsigned char>(state & 0xFFu);
    }

    return assets.CreateTexture(kGrainKey, kGrainSize, kGrainSize, pixels.data(), 4);
}
