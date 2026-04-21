#pragma once
#include "framebuffer.hpp"
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
        bool visible;
    };

    Surface(int res_x, int res_y) : m_rx(res_x), m_ry(res_y) {
        m_points.resize(res_x * res_y);
    }

    void update(std::function<float(float, float)> func, float x_min, float x_max, float y_min, float y_max) {
        for (int j = 0; j < m_ry; ++j) {
            float fy = y_min + (float)j / (m_ry - 1) * (y_max - y_min);
            for (int i = 0; i < m_rx; ++i) {
                float fx = x_min + (float)i / (m_rx - 1) * (x_max - x_min);
                m_points[j * m_rx + i].world = {fx, func(fx, fy), fy};
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
        // Multi-threaded draw: split the mesh into vertical chunks
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::future<void>> futures;
        int chunk_size = m_ry / num_threads;

        for (int t = 0; t < num_threads; ++t) {
            int start_y = t * chunk_size;
            int end_y = (t == num_threads - 1) ? m_ry : (t + 1) * chunk_size;

            futures.push_back(std::async(std::launch::async, [this, &fb, start_y, end_y, color]() {
                for (int j = start_y; j < end_y; ++j) {
                    for (int i = 0; i < m_rx; ++i) {
                        if (i < m_rx - 1) draw_edge(fb, j * m_rx + i, j * m_rx + i + 1, color);
                        if (j < m_ry - 1) draw_edge(fb, j * m_rx + i, (j + 1) * m_rx + i, color);
                    }
                }
            }));
        }
        for (auto& f : futures) f.wait();
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
