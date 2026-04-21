#pragma once
#include <linux/fb.h>
#include <cstdint>
#include "types.hpp"

namespace core {

class Framebuffer {
public:
    Framebuffer(const char* device_path = "/dev/fb0");
    ~Framebuffer();

    bool is_valid() const { return m_fb_ptr != nullptr; }

    uint32_t get_width() const { return m_vinfo.xres; }
    uint32_t get_height() const { return m_vinfo.yres; }
    uint32_t get_bpp() const { return m_vinfo.bits_per_pixel; }

    void put_pixel(uint32_t x, uint32_t y, const Color& color);
    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const Color& color);
    void clear(const Color& color);
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
