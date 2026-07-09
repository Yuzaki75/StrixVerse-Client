#pragma once

#include <cstdint>

class Color
{
public:
    float r;
    float g;
    float b;
    float a;

public:
    constexpr Color()
        : r(1.0f),
          g(1.0f),
          b(1.0f),
          a(1.0f)
    {
    }

    constexpr Color(
        float red,
        float green,
        float blue,
        float alpha = 1.0f)
        : r(red),
          g(green),
          b(blue),
          a(alpha)
    {
    }

public:
    static Color FromBytes(
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a = 255);

    static Color Lerp(
        const Color& start,
        const Color& end,
        float t);

public:
    static const Color White;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;

    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;

    static const Color Gray;
    static const Color Orange;
    static const Color Purple;

    static const Color Transparent;
};