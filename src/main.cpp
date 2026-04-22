#include "core/drm_display.hpp"
#include "core/surface.hpp"
#include "core/mesh_library.hpp"
#include "core/font.hpp"
#include "math/mat4.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>
#include <sstream>

int main() {
    core::DrmDisplay display("/dev/dri/card1");
    if (!display.is_valid()) {
        std::cerr << "Error: Could not initialize DRM device /dev/dri/card1." << std::endl;
        return 1;
    }

    input::InputManager input;
    core::Surface surface(80, 80); 
    auto& library = core::MeshLibrary::get();
    std::string current_preset = "torus";

    math::Vec3 cam_pos(0, 10, 25);
    float yaw = -1.57f, pitch = -0.3f;
    float time = 0;

    // Generate background stars
    struct Star { float x, y, z; uint32_t c; };
    std::vector<Star> stars;
    for (int i = 0; i < 200; ++i) {
        stars.push_back({
            (float)(rand() % 200 - 100),
            (float)(rand() % 200 - 100),
            (float)(rand() % 200 - 100),
            (uint32_t)(0xFF000000 | ((rand() % 155 + 100) << 16) | ((rand() % 155 + 100) << 8) | (rand() % 155 + 100))
        });
    }

    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    float fps = 0;
    auto fps_timer = last_time;
    
    while (!input.get_key_state(KEY_ESC)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        time += dt;
        frame_count++;

        if (std::chrono::duration<float>(now - fps_timer).count() >= 1.0f) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = now;
        }

        input.update();

        if (input.get_key_state(KEY_1)) current_preset = "ripple";
        if (input.get_key_state(KEY_2)) current_preset = "torus";
        if (input.get_key_state(KEY_3)) current_preset = "mobius";
        if (input.get_key_state(KEY_4)) current_preset = "klein";
        if (input.get_key_state(KEY_5)) current_preset = "plane";
        if (input.get_key_state(KEY_6)) current_preset = "beast";

        float speed = 15.0f * dt;
        float sens_kbd = 2.5f * dt;
        float sens_mouse = 0.005f;

        // Mouse rotation
        yaw += input.get_mouse_dx() * sens_mouse;
        pitch -= input.get_mouse_dy() * sens_mouse;

        math::Vec3 forward(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
        math::Vec3 right = forward.cross({0, 1, 0}).normalize();

        if (input.get_key_state(KEY_W)) cam_pos = cam_pos + forward * speed;
        if (input.get_key_state(KEY_S)) cam_pos = cam_pos - forward * speed;
        if (input.get_key_state(KEY_A)) cam_pos = cam_pos - right * speed;
        if (input.get_key_state(KEY_D)) cam_pos = cam_pos + right * speed;
        if (input.get_key_state(KEY_UP)) pitch += sens_kbd;
        if (input.get_key_state(KEY_DOWN)) pitch -= sens_kbd;
        if (input.get_key_state(KEY_LEFT)) yaw -= sens_kbd;
        if (input.get_key_state(KEY_RIGHT)) yaw += sens_kbd;

        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;

        display.clear(core::Color::BLACK);

        math::Mat4 view = math::Mat4::lookAt(cam_pos, cam_pos + forward, {0, 1, 0});
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)display.width()/display.height(), 0.1f, 200.0f);
        math::Mat4 mvp = proj * view;

        // Draw Stars
        for (const auto& s : stars) {
            math::Vec4 p4 = mvp * math::Vec4(s.x, s.y, s.z, 1.0f);
            if (p4.w > 0.1f) {
                math::Vec3 ndc = p4.to_vec3();
                int sx = (ndc.x + 1.0f) * 0.5f * display.width();
                int sy = (1.0f - (ndc.y + 1.0f) * 0.5f) * display.height();
                display.put_pixel(sx, sy, s.c);
            }
        }

        auto range = library.get_range(current_preset);
        surface.update([&](float u, float v) {
            return library.evaluate(current_preset, u, v, time);
        }, range.u_min, range.u_max, range.v_min, range.v_max, time);

        surface.project(mvp, display.width(), display.height());
        surface.draw(display);

        // Draw HUD
        std::stringstream ss;
        ss << "FPS: " << (int)fps << " PRESET: " << current_preset;
        core::FontRenderer::draw_text(display, 20, 20, ss.str(), core::Color::WHITE.to_u32(), 3);
        core::FontRenderer::draw_text(display, 20, 50, "WASD: MOVE  MOUSE: LOOK  1-6: PRESETS", 0xFFAAAAAA, 2);

        display.update();
    }

    display.clear(core::Color::BLACK);
    display.update();
    return 0;
}
