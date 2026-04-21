#include "core/drm_display.hpp"
#include "core/surface.hpp"
#include "core/mesh_library.hpp"
#include "math/mat4.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

int main() {
    core::DrmDisplay display("/dev/dri/card0");
    if (!display.is_valid()) return 1;

    input::InputManager input;
    core::Surface surface(60, 60); 
    auto& library = core::MeshLibrary::get();
    std::string current_preset = "ripple";

    math::Vec3 cam_pos(0, 8, 15);
    float yaw = -1.57f, pitch = -0.5f;
    float time = 0;

    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    auto fps_timer = last_time;
    
    while (!input.get_key_state(KEY_ESC)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        time += dt;
        frame_count++;

        if (std::chrono::duration<float>(now - fps_timer).count() >= 1.0f) {
            std::cout << "FPS: " << frame_count << std::endl;
            frame_count = 0;
            fps_timer = now;
        }

        input.update();

        if (input.get_key_state(KEY_1)) current_preset = "ripple";
        if (input.get_key_state(KEY_2)) current_preset = "saddle";
        if (input.get_key_state(KEY_3)) current_preset = "waves";

        float speed = 10.0f * dt;
        float sens = 2.5f * dt;

        math::Vec3 forward(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
        math::Vec3 right = forward.cross({0, 1, 0}).normalize();

        if (input.get_key_state(KEY_W)) cam_pos = cam_pos + forward * speed;
        if (input.get_key_state(KEY_S)) cam_pos = cam_pos - forward * speed;
        if (input.get_key_state(KEY_A)) cam_pos = cam_pos - right * speed;
        if (input.get_key_state(KEY_D)) cam_pos = cam_pos + right * speed;
        if (input.get_key_state(KEY_UP)) pitch += sens;
        if (input.get_key_state(KEY_DOWN)) pitch -= sens;
        if (input.get_key_state(KEY_LEFT)) yaw -= sens;
        if (input.get_key_state(KEY_RIGHT)) yaw += sens;

        display.clear(core::Color::BLACK);

        math::Mat4 view = math::Mat4::lookAt(cam_pos, cam_pos + forward, {0, 1, 0});
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)display.width()/display.height(), 0.1f, 100.0f);
        math::Mat4 mvp = proj * view;

        surface.update([&](float x, float y) {
            return library.evaluate(current_preset, x, y, time);
        }, -15, 15, -15, 15);

        surface.project(mvp, display.width(), display.height());
        surface.draw(display, core::Color::GREEN.to_u32());

        display.update(); // Synchronized to monitor refresh rate
    }

    display.clear(core::Color::BLACK);
    display.update();
    return 0;
}
