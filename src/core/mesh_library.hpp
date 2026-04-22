/*
 * MESH DEFINITION REGISTRY
 * A centralized repository of mathematical manifolds. 
 * Supports hot-swapping of parametric logic and heightmap evaluation.
 */

#pragma once
#include "surface.hpp"
#include <map>
#include <string>
#include <memory>
#include <cmath>

namespace core {

class MeshLibrary {
public:
    // Definition of the parametric search space (u,v domain)
    struct Range { float u_min, u_max, v_min, v_max; };
    using Func = std::function<math::Vec3(float, float, float)>;

    static MeshLibrary& get() {
        static MeshLibrary instance;
        return instance;
    }

    void add_preset(const std::string& name, Func f, Range r) {
        m_presets[name] = {f, r};
    }

    math::Vec3 evaluate(const std::string& name, float u, float v, float t) {
        if (m_presets.count(name)) return m_presets[name].f(u, v, t);
        return {0, 0, 0};
    }

    Range get_range(const std::string& name) {
        if (m_presets.count(name)) return m_presets[name].r;
        return {-10, 10, -10, 10};
    }

    void update_preset(const std::string& name, Func f) {
        if (m_presets.count(name)) {
            m_presets[name].f = f;
        }
    }

private:
    struct Entry { Func f; Range r; };

    MeshLibrary() {
        // Classic Ripple (Radial Sine Wave)
        add_preset("ripple", [](float x, float y, float t) {
            float d = std::sqrt(x*x + y*y);
            return math::Vec3(x, std::sin(d - t * 3.0f) * 2.0f, y);
        }, {-15, 15, -15, 15});

        // Torus (Parametric Ring)
        add_preset("torus", [](float u, float v, float t) {
            float R = 8.0f;
            float r = 3.0f + std::sin(t * 2.0f) * 0.5f;
            return math::Vec3(
                (R + r * std::cos(v)) * std::cos(u),
                r * std::sin(v),
                (R + r * std::cos(v)) * std::sin(u)
            );
        }, {0, 2 * M_PI, 0, 2 * M_PI});

        // Mobius Strip (Non-orientable manifold)
        add_preset("mobius", [](float u, float v, float t) {
            float a = 6.0f;
            float x = (a + v/2.0f * std::cos(u/2.0f)) * std::cos(u + t);
            float y = v/2.0f * std::sin(u/2.0f);
            float z = (a + v/2.0f * std::cos(u/2.0f)) * std::sin(u + t);
            return math::Vec3(x, y, z);
        }, {0, 2 * M_PI, -3, 3});

        // Klein Bottle (Non-orientable topology)
        add_preset("klein", [](float u, float v, float t) {
            float a = 6.0f;
            float x = (a + std::cos(u/2.0f) * std::sin(v) - std::sin(u/2.0f) * std::sin(2.0f*v)) * std::cos(u + t);
            float y = std::sin(u/2.0f) * std::sin(v) + std::cos(u/2.0f) * std::sin(2.0f*v);
            float z = (a + std::cos(u/2.0f) * std::sin(v) - std::sin(u/2.0f) * std::sin(2.0f*v)) * std::sin(u + t);
            return math::Vec3(x, y, z);
        }, {0, 2 * M_PI, 0, 2 * M_PI});

        // 3D Coordinate Plane / Sandbox (Identity Projection)
        add_preset("plane", [](float x, float y, float t) {
            // Visualize a flat grid that shows function z = f(x, y, t)
            float z = std::sin(x + t) * std::cos(y + t);
            return math::Vec3(x, z, y);
        }, {-15, 15, -15, 15});

        // "The Beast" (Harmonic oscillation manifold)
        add_preset("beast", [](float u, float v, float t) {
            float r = 5.0f * (1.0f + 0.3f * std::sin(8.0f * u + t));
            float x = r * std::cos(u) * std::sin(v);
            float y = r * std::sin(u) * std::sin(v) + std::cos(3.0f * v + t);
            float z = r * std::cos(v);
            return math::Vec3(x, y, z);
        }, {0, 2 * M_PI, 0, M_PI});
    }

    std::map<std::string, Entry> m_presets;
};

} // namespace core
