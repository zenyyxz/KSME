#pragma once
#include <linux/fb.h>
#include <cstdint>
#include <cstring>
#include "types.hpp"

namespace core {

class Framebuffer {
public:
    Framebuffer(const char* dev = "/dev/fb0");
    ~Framebuffer();

    bool is_valid() const { return m_fb_ptr != nullptr; }
    uint32_t width() const { return m_vinfo.xres; }
    uint32_t height() const { return m_vinfo.yres; }

    inline void put_pixel(int32_t x, int32_t y, uint32_t color) {
        if (x < 0 || y < 0 || x >= (int32_t)m_vinfo.xres || y >= (int32_t)m_vinfo.yres) return;
        *(uint32_t*)(m_backbuffer + y * m_finfo.line_length + (x << 2)) = color;
    }

    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void clear(const Color& c);
    void update();

private:
    int m_fd;
    fb_var_screeninfo m_vinfo;
    fb_fix_screeninfo m_finfo;
    uint8_t* m_fb_ptr;
    uint8_t* m_backbuffer;
    long m_screensize;
};

} // namespace core
