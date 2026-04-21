#pragma once
#include "framebuffer.hpp"
#include "../math/mat4.hpp"
#include <vector>
#include <functional>

namespace core {

class Surface {
public:
    struct Point {
        math::Vec3 world;
        math::Vec3 projected;
        bool visible;
    };

    Surface(int res_x, int res_y) : m_rx(res_x), m_ry(res_y) {
        m_points.resize(res_x * res_y);
    }

    void update(std::function<float(float, float)> func, float x_min, float x_max, float y_min, float y_max) {
        for (int j = 0; j < m_ry; ++j) {
            for (int i = 0; i < m_rx; ++i) {
                float fx = x_min + (float)i / (m_rx - 1) * (x_max - x_min);
                float fy = y_min + (float)j / (m_ry - 1) * (y_max - y_min);
                float fz = func(fx, fy);
                m_points[j * m_rx + i].world = {fx, fz, fy};
            }
        }
    }

    void project(const math::Mat4& mvp, int screen_w, int screen_h) {
        for (auto& p : m_points) {
            math::Vec4 p4 = mvp * math::Vec4(p.world, 1.0f);
            if (p4.w > 0.1f) {
                math::Vec3 ndc = p4.to_vec3();
                p.projected = {
                    (ndc.x + 1.0f) * 0.5f * screen_w,
                    (1.0f - (ndc.y + 1.0f) * 0.5f) * screen_h,
                    ndc.z
                };
                p.visible = true;
            } else {
                p.visible = false;
            }
        }
    }

    void draw(Framebuffer& fb, uint32_t color) {
        for (int j = 0; j < m_ry; ++j) {
            for (int i = 0; i < m_rx; ++i) {
                if (i < m_rx - 1) {
                    draw_edge(fb, j * m_rx + i, j * m_rx + i + 1, color);
                }
                if (j < m_ry - 1) {
                    draw_edge(fb, j * m_rx + i, (j + 1) * m_rx + i, color);
                }
            }
        }
    }

private:
    void draw_edge(Framebuffer& fb, int i1, int i2, uint32_t color) {
        const auto& p1 = m_points[i1];
        const auto& p2 = m_points[i2];
        if (p1.visible && p2.visible) {
            fb.draw_line((int)p1.projected.x, (int)p1.projected.y, 
                         (int)p2.projected.x, (int)p2.projected.y, color);
        }
    }

    int m_rx, m_ry;
    std::vector<Point> m_points;
};

} // namespace core
