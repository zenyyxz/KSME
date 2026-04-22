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
                float u_min, float u_max, float v_min, float v_max, float time) {
        // 1. Calculate world positions
        for (int j = 0; j < m_rv; ++j) {
            float fv = v_min + (float)j / (m_rv - 1) * (v_max - v_min);
            for (int i = 0; i < m_ru; ++i) {
                float fu = u_min + (float)i / (m_ru - 1) * (u_max - u_min);
                m_points[j * m_ru + i].world = func(fu, fv);
            }
        }

        // 2. Calculate normals and colors with dynamic lighting
        // Rotating light source for dynamic shadows
        math::Vec3 light_dir = math::Vec3(std::cos(time), 1.0f, std::sin(time)).normalize();
        
        for (int j = 0; j < m_rv; ++j) {
            for (int i = 0; i < m_ru; ++i) {
                auto& p = m_points[j * m_ru + i];
                
                // Better normal estimation using central differences where possible
                math::Vec3 v_u, v_v;
                if (i < m_ru - 1) v_u = m_points[j * m_ru + i + 1].world - p.world;
                else v_u = p.world - m_points[j * m_ru + i - 1].world;
                
                if (j < m_rv - 1) v_v = m_points[(j + 1) * m_ru + i].world - p.world;
                else v_v = p.world - m_points[(j - 1) * m_ru + i].world;
                
                math::Vec3 normal = v_u.cross(v_v).normalize();

                // Two-sided lighting (more visible wireframes)
                float dot = std::abs(normal.dot(light_dir));
                float intensity = 0.3f + 0.7f * dot; // Higher ambient

                // Enhanced color mapping: mix height and radial distance
                float dist = p.world.length() * 0.1f;
                float h = (p.world.y + 4.0f) * 0.15f; // Scale height for better range
                
                float factor = std::sin(h + dist + time) * 0.5f + 0.5f;
                
                uint8_t r = (uint8_t)((0.5f + 0.5f * std::sin(factor * 3.0f)) * 255 * intensity);
                uint8_t g = (uint8_t)((0.5f + 0.5f * std::cos(factor * 2.0f)) * 255 * intensity);
                uint8_t b = (uint8_t)((0.5f + 0.5f * std::sin(factor * 1.0f + 2.0f)) * 255 * intensity);
                
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
