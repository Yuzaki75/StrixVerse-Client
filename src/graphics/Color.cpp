#include "Color.h"

#include <algorithm>

const Color Color::White       = Color(1,1,1,1);
const Color Color::Black       = Color(0,0,0,1);

const Color Color::Red         = Color(1,0,0,1);
const Color Color::Green       = Color(0,1,0,1);
const Color Color::Blue        = Color(0,0,1,1);

const Color Color::Yellow      = Color(1,1,0,1);
const Color Color::Cyan        = Color(0,1,1,1);
const Color Color::Magenta     = Color(1,0,1,1);

const Color Color::Gray        = Color(.5f,.5f,.5f,1);
const Color Color::Orange      = Color(1,.5f,0,1);
const Color Color::Purple      = Color(.5f,0,.5f,1);

const Color Color::Transparent = Color(0,0,0,0);

Color Color::FromBytes(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a)
{
    return Color(
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f);
}

Color Color::Lerp(
    const Color& start,
    const Color& end,
    float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    return Color(
        start.r + (end.r - start.r) * t,
        start.g + (end.g - start.g) * t,
        start.b + (end.b - start.b) * t,
        start.a + (end.a - start.a) * t);
}