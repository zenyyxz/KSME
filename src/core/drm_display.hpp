#pragma once
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <cstdint>
#include <cstring>
#include "types.hpp"

namespace core {

class DrmDisplay {
public:
    struct Buffer {
        uint32_t handle;
        uint32_t fb_id;
        uint32_t stride;
        uint32_t size;
        uint8_t* map;
    };

    DrmDisplay(const char* device = "/dev/dri/card0");
    ~DrmDisplay();

    bool is_valid() const { return m_fd >= 0 && m_active_fb_id != 0; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    inline void put_pixel(int32_t x, int32_t y, uint32_t color) {
        if (x < 0 || y < 0 || x >= (int32_t)m_width || y >= (int32_t)m_height) return;
        *(uint32_t*)(m_buffers[m_back_idx].map + y * m_buffers[m_back_idx].stride + (x << 2)) = color;
    }

    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void clear(const Color& c);
    void update();

private:
    int m_fd;
    uint32_t m_width, m_height;
    uint32_t m_crtc_id;
    uint32_t m_conn_id;
    uint32_t m_encoder_id;
    
    // Manual structs for mode info
    struct drm_mode_modeinfo m_mode;
    
    Buffer m_buffers[2];
    int m_back_idx;
    uint32_t m_active_fb_id;

    bool init_drm();
    void create_buffer(Buffer& buf);
    void cleanup_drm();
};

} // namespace core
