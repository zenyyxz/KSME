#pragma once
#include <cstdint>

namespace core {

struct Color {
    uint8_t r, g, b, a;

    static Color from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return {r, g, b, a};
    }

    uint32_t to_u32() const {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color WHITE;
    static const Color BLACK;
};

inline const Color Color::RED = {255, 0, 0, 255};
inline const Color Color::GREEN = {0, 255, 0, 255};
inline const Color Color::BLUE = {0, 0, 255, 255};
inline const Color Color::WHITE = {255, 255, 255, 255};
inline const Color Color::BLACK = {0, 0, 0, 255};

} // namespace core
