#pragma once
#include "framebuffer.hpp"
#include <functional>
#include <cmath>

namespace core {

class Graph {
public:
    Graph(uint32_t w, uint32_t h) : m_w(w), m_h(h) {}

    void draw_2d(Framebuffer& fb, std::function<float(float)> func, float x_min, float x_max, float y_min, float y_max, uint32_t c) {
        float px = -1, py = -1;
        for (uint32_t i = 0; i < m_w; ++i) {
            float x = x_min + (float)i / m_w * (x_max - x_min);
            float y = func(x);
            float sx = (float)i;
            float sy = (1.0f - (y - y_min) / (y_max - y_min)) * m_h;
            if (px != -1) fb.draw_line((int)px, (int)py, (int)sx, (int)sy, c);
            px = sx; py = sy;
        }
    }

    void draw_axes(Framebuffer& fb, float x_min, float x_max, float y_min, float y_max, uint32_t c) {
        if (y_min <= 0 && y_max >= 0) {
            int y = (int)((1.0f - (0 - y_min) / (y_max - y_min)) * m_h);
            fb.draw_line(0, y, m_w, y, c);
        }
        if (x_min <= 0 && x_max >= 0) {
            int x = (int)((0 - x_min) / (x_max - x_min) * m_w);
            fb.draw_line(x, 0, x, m_h, c);
        }
    }

private:
    uint32_t m_w, m_h;
};

} // namespace core
