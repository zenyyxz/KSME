/*
 * MATH ENGINE ENTRY POINT
 * Coordinates hardware display, input events, and mathematical evaluation.
 */

#include "core/drm_display.hpp"
#include "core/surface.hpp"
#include "core/mesh_library.hpp"
#include "core/font.hpp"
#include "math/mat4.hpp"
#include "math/parser.hpp"
#include "input/input_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <termios.h>

enum class UIState { Normal, Input };
enum class PlotMode { Surface, Line };

int main() {
    /* 
     * TERMINAL INITIALIZATION
     * Suspends standard shell echoing to prevent input leakage during camera translation.
     */
    struct termios old_t, new_t;
    tcgetattr(STDIN_FILENO, &old_t);
    new_t = old_t;
    new_t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

    {
        /*
         * DISPLAY INITIALIZATION
         * Acquires DRM Mastership for direct hardware memory mapping.
         */
        core::DrmDisplay display("/dev/dri/card1");
        if (!display.is_valid()) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
            std::cerr << "Error: Could not initialize DRM device /dev/dri/card1." << std::endl;
            return 1;
        }

    input::InputManager input;
    core::Surface surface(120, 120); 
    auto& library = core::MeshLibrary::get();
    std::string current_preset = "torus";
    std::string active_formula = "sin(x + t) * cos(y + t)";
    UIState ui_state = UIState::Normal;
    PlotMode plot_mode = PlotMode::Surface;
    std::string input_buffer = "";

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
        time += dt; // Always update time
        frame_count++;

        // TELEMETRY: Calculate smoothed FPS
        if (std::chrono::duration<float>(now - fps_timer).count() >= 1.0f) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = now;
        }

        input.update();

        /*
         * STATE MACHINE: NORMAL NAVIGATION MODE
         */
        if (ui_state == UIState::Normal) {
            if (input.get_key_state(KEY_1)) current_preset = "ripple";
            if (input.get_key_state(KEY_2)) current_preset = "torus";
            if (input.get_key_state(KEY_3)) current_preset = "mobius";
            if (input.get_key_state(KEY_4)) current_preset = "klein";
            if (input.get_key_state(KEY_5)) current_preset = "plane";
            if (input.get_key_state(KEY_6)) current_preset = "beast";

            if (current_preset == "plane") {
                if (input.get_last_char() == 'i') {
                    ui_state = UIState::Input;
                    input_buffer = "";
                }
                if (input.get_last_char() == 'm') {
                    plot_mode = (plot_mode == PlotMode::Surface) ? PlotMode::Line : PlotMode::Surface;
                }
            }

            float speed = 15.0f * dt;
            float sens_kbd = 2.5f * dt;
            float sens_mouse = 0.005f;

            // CAMERA TRANSFORMATIONS: First-person spherical coordinates
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
        } else {
            /*
             * STATE MACHINE: FORMULA COMPILATION MODE
             */
            char c = input.get_last_char();
            bool ctrl = input.get_key_state(KEY_LEFTCTRL) || input.get_key_state(KEY_RIGHTCTRL);
            
            if (ctrl && input.get_key_state(KEY_X)) {
                auto func = math::Parser::parse(input_buffer);
                library.update_preset("plane", [func](float x, float y, float t) {
                    return math::Vec3(x, func(x, y, t), y);
                });
                active_formula = input_buffer;
                ui_state = UIState::Normal;
            } else if (c == '\n') {
                auto func = math::Parser::parse(input_buffer);
                library.update_preset("plane", [func](float x, float y, float t) {
                    return math::Vec3(x, func(x, y, t), y);
                });
                active_formula = input_buffer;
                ui_state = UIState::Normal;
            } else if (c == '\b') {
                if (!input_buffer.empty()) input_buffer.pop_back();
            } else if (c != 0) {
                // Only add printable ASCII
                if (c >= 32 && c <= 126) {
                    input_buffer += c;
                }
            }
            
            if (input.get_key_state(KEY_ESC)) ui_state = UIState::Normal;
        }

        display.clear(core::Color::BLACK);

        /*
         * RENDERING PIPELINE: MVP Matrix Generation
         */
        math::Vec3 forward_vec(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
        math::Mat4 view = math::Mat4::lookAt(cam_pos, cam_pos + forward_vec, {0, 1, 0});
        math::Mat4 proj = math::Mat4::perspective(1.0f, (float)display.width()/display.height(), 0.1f, 200.0f);
        math::Mat4 mvp = proj * view;

        auto project_to_screen = [&](math::Vec3 p) -> math::Vec3 {
            math::Vec4 p4 = mvp * math::Vec4(p, 1.0f);
            if (p4.w <= 0.1f) return {-1, -1, -1};
            math::Vec3 ndc = p4.to_vec3();
            return {
                (ndc.x + 1.0f) * 0.5f * display.width(),
                (1.0f - (ndc.y + 1.0f) * 0.5f) * display.height(),
                ndc.z
            };
        };

        // Draw Stars
        for (const auto& s : stars) {
            math::Vec3 sp = project_to_screen({s.x, s.y, s.z});
            if (sp.x != -1) display.put_pixel(sp.x, sp.y, s.c);
        }

        if (current_preset == "plane") {
            // Draw 3D Axes
            math::Vec3 origin = project_to_screen({0,0,0});
            math::Vec3 x_end = project_to_screen({20,0,0});
            math::Vec3 y_end = project_to_screen({0,20,0});
            math::Vec3 z_end = project_to_screen({0,0,20});

            if (origin.x != -1) {
                if (x_end.x != -1) {
                    display.draw_line(origin.x, origin.y, x_end.x, x_end.y, 0xFFFF0000); // Red X
                    core::FontRenderer::draw_text(display, x_end.x, x_end.y, "X", 0xFFFF0000, 2);
                }
                if (y_end.x != -1) {
                    display.draw_line(origin.x, origin.y, y_end.x, y_end.y, 0xFF00FF00); // Green Y
                    core::FontRenderer::draw_text(display, y_end.x, y_end.y, "Y", 0xFF00FF00, 2);
                }
                if (z_end.x != -1) {
                    display.draw_line(origin.x, origin.y, z_end.x, z_end.y, 0xFF0000FF); // Blue Z
                    core::FontRenderer::draw_text(display, z_end.x, z_end.y, "Z", 0xFF0000FF, 2);
                }
            }

            // Draw Floor Grid (XZ Plane)
            for (int i = -15; i <= 15; i += 5) {
                math::Vec3 s1 = project_to_screen({(float)i, 0, -15});
                math::Vec3 e1 = project_to_screen({(float)i, 0, 15});
                if (s1.x != -1 && e1.x != -1) display.draw_line(s1.x, s1.y, e1.x, e1.y, 0xFF444444);

                math::Vec3 s2 = project_to_screen({-15, 0, (float)i});
                math::Vec3 e2 = project_to_screen({15, 0, (float)i});
                if (s2.x != -1 && e2.x != -1) display.draw_line(s2.x, s2.y, e2.x, e2.y, 0xFF444444);
            }
        }

        /*
         * MESH EVALUATION
         * Hot-swaps vertex logic based on the active AST from the math parser.
         */
        if (current_preset == "plane" && plot_mode == PlotMode::Line) {
            // High-res 1D line plotter in 3D space
            auto func = math::Parser::parse(active_formula);
            math::Vec3 prev = {-1, -1, -1};
            float prev_y_val = 0;
            for (float x = -15; x <= 15; x += 0.02f) {
                float y_val = func(x, 0, time);
                
                // Asymptote detection: if Y jump is huge, it's likely a division by zero
                bool skip = (prev.x != -1 && std::abs(y_val - prev_y_val) > 40.0f);
                
                math::Vec3 p = project_to_screen({x, y_val, 0});
                if (p.x != -1 && prev.x != -1 && !skip) {
                    display.draw_line(prev.x, prev.y, p.x, p.y, core::Color::GREEN.to_u32());
                }
                prev = skip ? math::Vec3(-1, -1, -1) : p;
                prev_y_val = y_val;
            }
        } else {
            auto range = library.get_range(current_preset);
            surface.update([&](float u, float v) {
                return library.evaluate(current_preset, u, v, time);
            }, range.u_min, range.u_max, range.v_min, range.v_max, time);

            surface.project(mvp, display.width(), display.height());
            surface.draw(display);
        }

        // Draw HUD
        std::stringstream ss;
        ss << "FPS: " << (int)fps << " PRESET: " << current_preset;
        if (current_preset == "plane") ss << " (" << (plot_mode == PlotMode::Surface ? "SURFACE" : "LINE") << ")";
        core::FontRenderer::draw_text(display, 20, 20, ss.str(), core::Color::WHITE.to_u32(), 3);
        
        if (ui_state == UIState::Normal) {
            core::FontRenderer::draw_text(display, 20, 50, "WASD: MOVE  MOUSE: LOOK  1-6: PRESETS", 0xFFAAAAAA, 2);
            if (current_preset == "plane") {
                core::FontRenderer::draw_text(display, 20, 80, "FORMULA: " + active_formula, core::Color::GREEN.to_u32(), 2);
                core::FontRenderer::draw_text(display, 20, 110, "PRESS 'I' TO EDIT  'M' TO TOGGLE MODE", core::Color::WHITE.to_u32(), 2);
            }
        } else {
            // Repositioned lower with internal spacing
            int bw = 600, bh = 100;
            int bx = 20;
            int by = 200;
            core::FontRenderer::draw_box(display, bx, by, bw, bh, core::Color::WHITE.to_u32(), "ENTER FORMULA");
            
            core::FontRenderer::draw_text(display, bx + 20, by + 30, "CTRL+X TO APPLY / ESC TO CANCEL", 0xFFAAAAAA, 2);
            core::FontRenderer::draw_text(display, bx + 20, by + 65, "> " + input_buffer + "_", core::Color::GREEN.to_u32(), 3);
        }

        display.update();
    }

        display.clear(core::Color::BLACK);
        display.update();
    } // release display object

    /*
     * ENVIRONMENT RESTORATION
     * Reverts terminal to standard operating state.
     */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
    return 0;
}
