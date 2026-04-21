#pragma once
#include "framebuffer.hpp"
#include "surface.hpp"
#include <map>
#include <string>
#include <memory>

namespace core {

class MeshLibrary {
public:
    using Func = std::function<float(float, float, float)>;

    static MeshLibrary& get() {
        static MeshLibrary instance;
        return instance;
    }

    void add_preset(const std::string& name, Func f) {
        m_presets[name] = f;
    }

    float evaluate(const std::string& name, float x, float y, float t) {
        if (m_presets.count(name)) return m_presets[name](x, y, t);
        return 0;
    }

private:
    MeshLibrary() {
        // The "Fire" Ripple
        add_preset("ripple", [](float x, float y, float t) {
            float d = std::sqrt(x*x + y*y);
            return std::sin(d - t * 3.0f) * 2.0f;
        });

        // Saddle / Pringle
        add_preset("saddle", [](float x, float y, float) {
            return (x*x - y*y) * 0.1f;
        });

        // Wave tank
        add_preset("waves", [](float x, float y, float t) {
            return std::sin(x + t) * std::cos(y + t);
        });
    }

    std::map<std::string, Func> m_presets;
};

} // namespace core
