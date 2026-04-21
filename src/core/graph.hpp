#pragma once
#include "framebuffer.hpp"
#include <vector>
#include <functional>
#include <cmath>

namespace core {

class Graph {
public:
    Graph(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}

    void draw_2d(Framebuffer& fb, std::function<float(float)> func, float x_min, float x_max, float y_min, float y_max, const Color& color) {
        float prev_screen_x = -1, prev_screen_y = -1;

        for (uint32_t i = 0; i < m_width; ++i) {
            float x = x_min + (float)i / m_width * (x_max - x_min);
            float y = func(x);

            float screen_x = (float)i;
            float screen_y = (1.0f - (y - y_min) / (y_max - y_min)) * m_height;

            if (prev_screen_x != -1) {
                fb.draw_line((int)prev_screen_x, (int)prev_screen_y, (int)screen_x, (int)screen_y, color);
            }

            prev_screen_x = screen_x;
            prev_screen_y = screen_y;
        }
    }

    void draw_axes(Framebuffer& fb, float x_min, float x_max, float y_min, float y_max, const Color& color) {
        // X-axis
        if (y_min <= 0 && y_max >= 0) {
            int y = (int)((1.0f - (0 - y_min) / (y_max - y_min)) * m_height);
            fb.draw_line(0, y, m_width, y, color);
        }
        // Y-axis
        if (x_min <= 0 && x_max >= 0) {
            int x = (int)((0 - x_min) / (x_max - x_min) * m_width);
            fb.draw_line(x, 0, x, m_height, color);
        }
    }

private:
    uint32_t m_width, m_height;
};

} // namespace core
