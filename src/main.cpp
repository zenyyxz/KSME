#include "core/framebuffer.hpp"
#include "core/surface.hpp"
#include "core/mesh_library.hpp"
#include "math/mat4.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

int main() {
    core::Framebuffer fb("/dev/fb0");
    if (!fb.is_valid()) return 1;

    input::InputManager input;
    core::Surface surface(45, 45); // Slightly higher res for the "weapon"
    auto& library = core::MeshLibrary::get();
    std::string current_preset = "ripple";

    math::Vec3 cam_pos(0, 8, 15);
    float yaw = -1.57f, pitch = -0.5f;
    float time = 0;

    auto last_time = std::chrono::high_resolution_clock::now();
    
    while (!input.get_key_state(KEY_ESC)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        time += dt;

        input.update();

        // Preset switching
        if (input.get_key_state(KEY_1)) current_preset = "ripple";
        if (input.get_key_state(KEY_2)) current_preset = "saddle";
        if (input.get_key_state(KEY_3)) current_preset = "waves";

        // Camera control (The smoother path)
        float speed = 8.0f * dt;
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

        fb.clear(core::Color::BLACK);

        math::Mat4 view = math::Mat4::lookAt(cam_pos, cam_pos + forward, {0, 1, 0});
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)fb.width()/fb.height(), 0.1f, 100.0f);
        math::Mat4 mvp = proj * view;

        surface.update([&](float x, float y) {
            return library.evaluate(current_preset, x, y, time);
        }, -15, 15, -15, 15);

        surface.project(mvp, fb.width(), fb.height());
        surface.draw(fb, core::Color::GREEN.to_u32());

        fb.update();

        // High-Performance Pacing: Hybrid Sleep + Busy Wait
        // This keeps the CPU from downclocking and eliminates "micro-stutter"
        const float target_dt = 1.0f / 60.0f;
        auto end_frame = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(end_frame - now).count();
        
        if (elapsed < target_dt) {
            // Sleep for the bulk of the time
            float sleep_time = (target_dt - elapsed) * 0.8f; 
            if (sleep_time > 0.001f) {
                std::this_thread::sleep_for(std::chrono::duration<float>(sleep_time));
            }
            // Busy wait for the last bit to stay precise and keep CPU clocks up
            while (std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - now).count() < target_dt) {
                __builtin_ia32_pause(); // Hint to CPU that we're in a spin loop
            }
        }
    }

    fb.clear(core::Color::BLACK);
    fb.update();
    return 0;
}
