#pragma once
#include "../math/mat4.hpp"
#include <vector>
#include <functional>
#include <thread>
#include <future>

namespace core {

class Surface {
public:
    struct Point {
        math::Vec3 world;
        math::Vec3 projected;
        uint32_t color;
        bool visible;
    };

    Surface(int res_u, int res_v) : m_ru(res_u), m_rv(res_v) {
        m_points.resize(res_u * res_v);
    }

    // Parametric update: func(u, v) -> Vec3(x, y, z)
    void update(std::function<math::Vec3(float, float)> func, 
                float u_min, float u_max, float v_min, float v_max) {
        for (int j = 0; j < m_rv; ++j) {
            float fv = v_min + (float)j / (m_rv - 1) * (v_max - v_min);
            for (int i = 0; i < m_ru; ++i) {
                float fu = u_min + (float)i / (m_ru - 1) * (u_max - u_min);
                auto& p = m_points[j * m_ru + i];
                p.world = func(fu, fv);
                
                // Dynamic height-based color (Blue -> Green -> Red)
                float h = (p.world.y + 5.0f) / 10.0f; // Normalized -5 to 5
                if (h < 0) h = 0; if (h > 1) h = 1;
                uint8_t r = (uint8_t)(h * 255);
                uint8_t g = (uint8_t)((1.0f - std::abs(h - 0.5f) * 2.0f) * 255);
                uint8_t b = (uint8_t)((1.0f - h) * 255);
                p.color = (255 << 24) | (r << 16) | (g << 8) | b;
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

    template<typename Display>
    void draw(Display& fb) {
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::future<void>> futures;
        int chunk_size = m_rv / num_threads;

        for (int t = 0; t < num_threads; ++t) {
            int start_v = t * chunk_size;
            int end_v = (t == num_threads - 1) ? m_rv : (t + 1) * chunk_size;

            futures.push_back(std::async(std::launch::async, [this, &fb, start_v, end_v]() {
                for (int j = start_v; j < end_v; ++j) {
                    for (int i = 0; i < m_ru; ++i) {
                        if (i < m_ru - 1) draw_edge(fb, j * m_ru + i, j * m_ru + i + 1);
                        if (j < m_rv - 1) draw_edge(fb, j * m_ru + i, (j + 1) * m_ru + i);
                    }
                }
            }));
        }
        for (auto& f : futures) f.wait();
    }

private:
    template<typename Display>
    void draw_edge(Display& fb, int i1, int i2) {
        const auto& p1 = m_points[i1];
        const auto& p2 = m_points[i2];
        if (p1.visible && p2.visible) {
            // Average color for the edge
            fb.draw_line((int)p1.projected.x, (int)p1.projected.y, 
                         (int)p2.projected.x, (int)p2.projected.y, p1.color);
        }
    }

    int m_ru, m_rv;
    std::vector<Point> m_points;
};

} // namespace core
