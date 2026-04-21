#include "core/framebuffer.hpp"
#include "core/graph.hpp"
#include "math/mat4.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

struct Edge {
    int v1, v2;
};

int main() {
    core::Framebuffer fb("/dev/fb0");
    if (!fb.is_valid()) return 1;

    input::InputManager input;
    core::Graph graph(fb.get_width(), fb.get_height());

    std::vector<math::Vec3> vertices = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
    };

    std::vector<Edge> edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    float angle = 0;
    auto last_time = std::chrono::high_resolution_clock::now();
    bool running = true;

    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        input.update();
        if (input.get_key_state(KEY_ESC)) running = false;

        angle += dt * 0.5f;

        fb.clear(core::Color::BLACK);

        // Draw 3D cube
        math::Mat4 model = math::Mat4::rotationY(angle) * math::Mat4::rotationX(angle * 0.3f);
        math::Mat4 view = math::Mat4::translation(2.0f, 0, -6); // Move cube to the right
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)fb.get_width() / fb.get_height(), 0.1f, 100.0f);
        math::Mat4 mvp = proj * view * model;

        std::vector<math::Vec3> projected_vertices;
        for (const auto& v : vertices) {
            math::Vec4 v4(v, 1.0f);
            math::Vec4 p4 = mvp * v4;
            math::Vec3 p = p4.to_vec3();
            float screen_x = (p.x + 1.0f) * 0.5f * fb.get_width();
            float screen_y = (1.0f - (p.y + 1.0f) * 0.5f) * fb.get_height();
            projected_vertices.push_back({screen_x, screen_y, 0});
        }

        for (const auto& edge : edges) {
            const auto& v1 = projected_vertices[edge.v1];
            const auto& v2 = projected_vertices[edge.v2];
            fb.draw_line((int)v1.x, (int)v1.y, (int)v2.x, (int)v2.y, core::Color::BLUE);
        }

        // Draw 2D sine wave on the left side
        graph.draw_axes(fb, -10, 10, -2, 2, core::Color::WHITE);
        graph.draw_2d(fb, [&](float x) { return std::sin(x + angle); }, -10, 10, -2, 2, core::Color::RED);

        fb.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    fb.clear(core::Color::BLACK);
    fb.update();
    return 0;
}
