#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace core {

struct VectorChar {
    struct Line { int x1, y1, x2, y2; };
    std::vector<Line> lines;
};

class FontRenderer {
public:
    template<typename T>
    static void draw_text(T& fb, int x, int y, const std::string& text, uint32_t color, int scale = 2) {
        int cur_x = x;
        for (char c : text) {
            draw_char(fb, cur_x, y, c, color, scale);
            cur_x += 6 * scale;
        }
    }

private:
    template<typename T>
    static void draw_char(T& fb, int x, int y, char c, uint32_t color, int scale) {
        // Convert lowercase to uppercase
        if (c >= 'a' && c <= 'z') c -= 32;

        auto draw_line = [&](int x1, int y1, int x2, int y2) {
            fb.draw_line(x + x1 * scale, y + y1 * scale, x + x2 * scale, y + y2 * scale, color);
        };

        switch (c) {
            case '0': draw_line(0,0,4,0); draw_line(4,0,4,6); draw_line(4,6,0,6); draw_line(0,6,0,0); break;
            case '1': draw_line(2,0,2,6); break;
            case '2': draw_line(0,0,4,0); draw_line(4,0,4,3); draw_line(4,3,0,3); draw_line(0,3,0,6); draw_line(0,6,4,6); break;
            case '3': draw_line(0,0,4,0); draw_line(4,0,4,6); draw_line(4,6,0,6); draw_line(0,3,4,3); break;
            case '4': draw_line(0,0,0,3); draw_line(0,3,4,3); draw_line(4,0,4,6); break;
            case '5': draw_line(4,0,0,0); draw_line(0,0,0,3); draw_line(0,3,4,3); draw_line(4,3,4,6); draw_line(4,6,0,6); break;
            case '6': draw_line(0,0,0,6); draw_line(0,6,4,6); draw_line(4,6,4,3); draw_line(4,3,0,3); break;
            case '7': draw_line(0,0,4,0); draw_line(4,0,4,6); break;
            case '8': draw_line(0,0,4,0); draw_line(4,0,4,6); draw_line(4,6,0,6); draw_line(0,6,0,0); draw_line(0,3,4,3); break;
            case '9': draw_line(4,3,0,3); draw_line(0,3,0,0); draw_line(0,0,4,0); draw_line(4,0,4,6); break;
            case ':': draw_line(2,1,2,1); draw_line(2,5,2,5); break;
            case '.': draw_line(2,6,2,6); break;
            case '-': draw_line(1,3,3,3); break;
            case 'A': draw_line(0,6,0,2); draw_line(0,2,2,0); draw_line(2,0,4,2); draw_line(4,2,4,6); draw_line(0,3,4,3); break;
            case 'B': draw_line(0,0,0,6); draw_line(0,0,3,0); draw_line(3,0,4,1); draw_line(4,1,4,2); draw_line(4,2,3,3); draw_line(3,3,0,3); draw_line(3,3,4,4); draw_line(4,4,4,5); draw_line(4,5,3,6); draw_line(3,6,0,6); break;
            case 'C': draw_line(4,0,0,0); draw_line(0,0,0,6); draw_line(0,6,4,6); break;
            case 'D': draw_line(0,0,0,6); draw_line(0,0,3,0); draw_line(3,0,4,2); draw_line(4,2,4,4); draw_line(4,4,3,6); draw_line(3,6,0,6); break;
            case 'E': draw_line(4,0,0,0); draw_line(0,0,0,6); draw_line(0,6,4,6); draw_line(0,3,3,3); break;
            case 'F': draw_line(0,0,0,6); draw_line(0,0,4,0); draw_line(0,3,3,3); break;
            case 'G': draw_line(4,0,0,0); draw_line(0,0,0,6); draw_line(0,6,4,6); draw_line(4,6,4,3); draw_line(4,3,2,3); break;
            case 'H': draw_line(0,0,0,6); draw_line(4,0,4,6); draw_line(0,3,4,3); break;
            case 'I': draw_line(2,0,2,6); draw_line(0,0,4,0); draw_line(0,6,4,6); break;
            case 'J': draw_line(4,0,4,5); draw_line(4,5,2,6); draw_line(2,6,0,5); break;
            case 'K': draw_line(0,0,0,6); draw_line(0,3,4,0); draw_line(0,3,4,6); break;
            case 'L': draw_line(0,0,0,6); draw_line(0,6,4,6); break;
            case 'M': draw_line(0,6,0,0); draw_line(0,0,2,2); draw_line(2,2,4,0); draw_line(4,0,4,6); break;
            case 'N': draw_line(0,6,0,0); draw_line(0,0,4,6); draw_line(4,6,4,0); break;
            case 'O': draw_line(0,0,4,0); draw_line(4,0,4,6); draw_line(4,6,0,6); draw_line(0,6,0,0); break;
            case 'P': draw_line(0,0,0,6); draw_line(0,0,4,0); draw_line(4,0,4,3); draw_line(4,3,0,3); break;
            case 'Q': draw_line(0,0,4,0); draw_line(4,0,4,6); draw_line(4,6,0,6); draw_line(0,6,0,0); draw_line(2,4,4,6); break;
            case 'R': draw_line(0,6,0,0); draw_line(0,0,4,0); draw_line(4,0,4,3); draw_line(4,3,0,3); draw_line(1,3,4,6); break;
            case 'S': draw_line(4,0,0,0); draw_line(0,0,0,3); draw_line(0,3,4,3); draw_line(4,3,4,6); draw_line(4,6,0,6); break;
            case 'T': draw_line(0,0,4,0); draw_line(2,0,2,6); break;
            case 'U': draw_line(0,0,0,6); draw_line(0,6,4,6); draw_line(4,6,4,0); break;
            case 'V': draw_line(0,0,2,6); draw_line(2,6,4,0); break;
            case 'W': draw_line(0,0,0,6); draw_line(0,6,2,4); draw_line(2,4,4,6); draw_line(4,6,4,0); break;
            case 'X': draw_line(0,0,4,6); draw_line(4,0,0,6); break;
            case 'Y': draw_line(0,0,2,3); draw_line(4,0,2,3); draw_line(2,3,2,6); break;
            case 'Z': draw_line(0,0,4,0); draw_line(4,0,0,6); draw_line(0,6,4,6); break;
            case ' ': break;
            default: draw_line(1,3,3,3); break; // Dash for unknown
        }
    }
};

} // namespace core