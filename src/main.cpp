#include "core/framebuffer.hpp"
#include "core/surface.hpp"
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
    core::Surface surface(40, 40);

    math::Vec3 cam_pos(0, 5, 10);
    float yaw = -1.57f, pitch = -0.4f;

    auto last_time = std::chrono::high_resolution_clock::now();
    
    while (!input.get_key_state(KEY_ESC)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        input.update();

        // Camera control
        float speed = 5.0f * dt;
        float sens = 2.0f * dt;

        if (input.get_key_state(KEY_W)) cam_pos = cam_pos + math::Vec3(std::cos(yaw), 0, std::sin(yaw)) * speed;
        if (input.get_key_state(KEY_S)) cam_pos = cam_pos - math::Vec3(std::cos(yaw), 0, std::sin(yaw)) * speed;
        if (input.get_key_state(KEY_A)) cam_pos = cam_pos - math::Vec3(-std::sin(yaw), 0, std::cos(yaw)) * speed;
        if (input.get_key_state(KEY_D)) cam_pos = cam_pos + math::Vec3(-std::sin(yaw), 0, std::cos(yaw)) * speed;
        if (input.get_key_state(KEY_UP)) pitch += sens;
        if (input.get_key_state(KEY_DOWN)) pitch -= sens;
        if (input.get_key_state(KEY_LEFT)) yaw -= sens;
        if (input.get_key_state(KEY_RIGHT)) yaw += sens;

        fb.clear(core::Color::BLACK);

        math::Vec3 forward(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
        math::Mat4 view = math::Mat4::lookAt(cam_pos, cam_pos + forward, {0, 1, 0});
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)fb.width()/fb.height(), 0.1f, 100.0f);
        math::Mat4 mvp = proj * view;

        // Animated function: Ripple effect
        static float time = 0;
        time += dt;
        surface.update([&](float x, float y) {
            float d = std::sqrt(x*x + y*y);
            return std::sin(d - time * 2.0f) * 2.0f;
        }, -10, 10, -10, 10);

        surface.project(mvp, fb.width(), fb.height());
        surface.draw(fb, core::Color::GREEN.to_u32());

        fb.update();

        auto end_frame = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(end_frame - now).count();
        if (elapsed < 0.016f) std::this_thread::sleep_for(std::chrono::duration<float>(0.016f - elapsed));
    }

    fb.clear(core::Color::BLACK);
    fb.update();
    return 0;
}
