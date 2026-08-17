#version 330 core

// -----------------------------------------------------------------------------
// StrixVerse UI vertex shader
//
// One vertex format drives every UI primitive: solid/gradient panels, rounded
// borders, outer glows, textured images and text glyphs. The fragment shader
// picks the behaviour from aStyle.w (flags), so an entire screen can be drawn
// in insertion order with a batch break only on texture or clip changes.
//
// Quad geometry is expanded outwards by the glow size; aRect always describes
// the logical (un-expanded) box so the signed-distance field stays correct.
// -----------------------------------------------------------------------------

layout(location = 0) in vec2 aPos;         // position, virtual-canvas pixels
layout(location = 1) in vec2 aUV;          // texture coordinates
layout(location = 2) in vec4 aFillTop;     // gradient start colour
layout(location = 3) in vec4 aFillBottom;  // gradient end colour
layout(location = 4) in vec4 aBorderColor;
layout(location = 5) in vec4 aGlowColor;
layout(location = 6) in vec4 aRect;        // logical box: x, y, width, height
layout(location = 7) in vec4 aStyle;       // radius, borderWidth, glowSize, flags
layout(location = 8) in vec2 aLocalPos;    // position before any rotation

uniform mat4 uProjection;

out vec2 vPos;
out vec2 vUV;
out vec4 vFillTop;
out vec4 vFillBottom;
out vec4 vBorderColor;
out vec4 vGlowColor;
out vec4 vRect;
out vec4 vStyle;

void main()
{
    // The distance field and the gradient are evaluated in the quad's own
    // unrotated space, so a rotated shard still gets correct rounded corners.
    vPos         = aLocalPos;
    vUV          = aUV;
    vFillTop     = aFillTop;
    vFillBottom  = aFillBottom;
    vBorderColor = aBorderColor;
    vGlowColor   = aGlowColor;
    vRect        = aRect;
    vStyle       = aStyle;

    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
