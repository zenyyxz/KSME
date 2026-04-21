#include "core/framebuffer.hpp"
#include "core/graph.hpp"
#include "math/mat4.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

struct Edge { int v1, v2; };

int main() {
    core::Framebuffer fb("/dev/fb0");
    if (!fb.is_valid()) return 1;

    input::InputManager input;
    core::Graph graph(fb.width(), fb.height());

    std::vector<math::Vec3> vertices = {
        {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
        {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
    };
    std::vector<Edge> edges = {
        {0,1}, {1,2}, {2,3}, {3,0}, {4,5}, {5,6}, {6,7}, {7,4}, {0,4}, {1,5}, {2,6}, {3,7}
    };

    float angle = 0;
    auto last_time = std::chrono::high_resolution_clock::now();
    
    while (!input.get_key_state(KEY_ESC)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        input.update();
        angle += dt * 0.8f;

        fb.clear(core::Color::BLACK);

        math::Mat4 model = math::Mat4::rotationY(angle) * math::Mat4::rotationX(angle * 0.4f);
        math::Mat4 view = math::Mat4::translation(2.0f, 0, -6);
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)fb.width()/fb.height(), 0.1f, 100.0f);
        math::Mat4 mvp = proj * view * model;

        std::vector<math::Vec3> proj_v;
        for (const auto& v : vertices) {
            math::Vec4 p4 = mvp * math::Vec4(v, 1.0f);
            math::Vec3 p = p4.to_vec3();
            proj_v.push_back({(p.x + 1.0f) * 0.5f * fb.width(), (1.0f - (p.y + 1.0f) * 0.5f) * fb.height(), 0});
        }

        for (const auto& e : edges) {
            fb.draw_line((int)proj_v[e.v1].x, (int)proj_v[e.v1].y, (int)proj_v[e.v2].x, (int)proj_v[e.v2].y, core::Color::BLUE);
        }

        graph.draw_axes(fb, -10, 10, -2, 2, core::Color::WHITE);
        graph.draw_2d(fb, [&](float x) { return std::sin(x + angle); }, -10, 10, -2, 2, core::Color::RED);

        fb.update();

        // Target 60fps - only sleep if we have time left
        auto end_time = std::chrono::high_resolution_clock::now();
        float frame_time = std::chrono::duration<float>(end_time - now).count();
        if (frame_time < 0.0166f) {
            std::this_thread::sleep_for(std::chrono::duration<float>(0.0166f - frame_time));
        }
    }

    fb.clear(core::Color::BLACK);
    fb.update();
    return 0;
}
