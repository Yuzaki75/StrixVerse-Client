#pragma once

#include <memory>

class AssetManager;
class Texture;

// -----------------------------------------------------------------------------
// UIPatterns
//
// The design's two full-bleed background patterns, generated once at startup
// and cached in the AssetManager rather than shipped as image files:
//
//   * pixel grid - ".sv-pixel-grid", a dot lattice behind the login, register,
//     connecting, continue and loading screens.
//   * grain      - the splash frame's "Noise & Texture" overlay.
//
// Both are deterministic, so they look identical on every run, and both are
// tiled by UITiledImage rather than stretched.
// -----------------------------------------------------------------------------
namespace UIPatterns
{
    // One repeat of the dot lattice. Tile at kPixelGridTileSize canvas pixels.
    std::shared_ptr<Texture> GetPixelGrid(AssetManager& assets);

    // One repeat of the film grain. Tile at kGrainTileSize canvas pixels.
    std::shared_ptr<Texture> GetGrain(AssetManager& assets);

    // ".sv-pixel-grid" uses a 24px lattice in style-guide pixels, which is
    // 24 * (1080/460) on the design canvas.
    inline constexpr float kPixelGridTileSize = 24.0f * (1080.0f / 460.0f);

    inline constexpr float kGrainTileSize = 512.0f;
}
