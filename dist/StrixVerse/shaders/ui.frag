#version 330 core

// -----------------------------------------------------------------------------
// StrixVerse UI fragment shader
//
// Reproduces the Figma "crystal" styling analytically instead of from exported
// bitmaps: rounded corners, a 1px inner border, an outer glow and a vertical
// gradient fill all come out of one signed-distance field evaluation.
//
// aStyle.w flags:
//   1 : sample uTexture as RGBA and modulate the fill (images)
//   2 : sample uTexture.r as coverage (font atlas glyphs)
//   4 : bypass the shape entirely (fill covers the whole quad)
// -----------------------------------------------------------------------------

in vec2 vPos;
in vec2 vUV;
in vec4 vFillTop;
in vec4 vFillBottom;
in vec4 vBorderColor;
in vec4 vGlowColor;
in vec4 vRect;
in vec4 vStyle;

uniform sampler2D uTexture;

out vec4 FragColor;

const int FLAG_TEXTURE = 1;
const int FLAG_GLYPH   = 2;
const int FLAG_NOSHAPE = 4;

// Signed distance to a rounded box centred on the origin.
// Negative inside, positive outside.
float sdRoundBox(vec2 p, vec2 halfSize, float radius)
{
    // A radius can never exceed half of the shortest side.
    float r = min(radius, min(halfSize.x, halfSize.y));
    vec2 q = abs(p) - halfSize + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;
}

void main()
{
    int   flags       = int(vStyle.w + 0.5);
    float radius      = vStyle.x;
    float borderWidth = vStyle.y;
    float glowSize    = vStyle.z;

    // Vertical gradient across the logical box (not the glow-expanded quad).
    float t = vRect.w > 0.0 ? clamp((vPos.y - vRect.y) / vRect.w, 0.0, 1.0) : 0.0;
    vec4 fill = mix(vFillTop, vFillBottom, t);

    if ((flags & FLAG_TEXTURE) != 0)
    {
        fill *= texture(uTexture, vUV);
    }
    else if ((flags & FLAG_GLYPH) != 0)
    {
        fill.a *= texture(uTexture, vUV).r;
    }

    if ((flags & FLAG_NOSHAPE) != 0)
    {
        FragColor = vec4(fill.rgb * fill.a, fill.a);
        return;
    }

    vec2  center   = vRect.xy + vRect.zw * 0.5;
    vec2  halfSize = vRect.zw * 0.5;
    float dist     = sdRoundBox(vPos - center, halfSize, radius);

    // Screen-space antialiasing width, so edges stay crisp at every UI scale.
    float aa = max(fwidth(dist), 0.0001);

    // Coverage of the shape itself.
    float shape = 1.0 - smoothstep(-aa, aa, dist);

    // Outer glow lives strictly outside the shape and falls off quadratically,
    // which matches the CSS box-shadow blur the design uses.
    float glow = 0.0;
    if (glowSize > 0.0)
    {
        float g = 1.0 - clamp(dist / glowSize, 0.0, 1.0);
        glow = g * g * step(0.0, dist) * vGlowColor.a;
    }

    // Border occupies the band [-borderWidth, 0] of the distance field.
    float border = 0.0;
    if (borderWidth > 0.0)
    {
        border = shape * smoothstep(-borderWidth - aa, -borderWidth + aa, dist);
    }

    // Composite back to front: glow, then fill, then border. Premultiplied so
    // overlapping translucent layers blend without darkening at the seams.
    vec3  rgb   = vGlowColor.rgb * glow;
    float alpha = glow;

    float fillA = fill.a * shape * (1.0 - border);
    rgb   = rgb * (1.0 - fillA) + fill.rgb * fillA;
    alpha = alpha * (1.0 - fillA) + fillA;

    float borderA = vBorderColor.a * border;
    rgb   = rgb * (1.0 - borderA) + vBorderColor.rgb * borderA;
    alpha = alpha * (1.0 - borderA) + borderA;

    if (alpha <= 0.0)
        discard;

    FragColor = vec4(rgb, alpha);
}
