#include "framebuffer.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

namespace core {

Framebuffer::Framebuffer(const char* dev) : m_fd(-1), m_fb_ptr(nullptr), m_backbuffer(nullptr) {
    m_fd = open(dev, O_RDWR);
    if (m_fd < 0) return;

    if (ioctl(m_fd, FBIOGET_FSCREENINFO, &m_finfo) < 0 ||
        ioctl(m_fd, FBIOGET_VSCREENINFO, &m_vinfo) < 0) {
        close(m_fd);
        m_fd = -1;
        return;
    }

    m_screensize = m_finfo.line_length * m_vinfo.yres;
    m_fb_ptr = (uint8_t*)mmap(0, m_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    
    if (m_fb_ptr == MAP_FAILED) {
        m_fb_ptr = nullptr;
        close(m_fd);
        return;
    }

    m_backbuffer = new uint8_t[m_screensize];
    memset(m_backbuffer, 0, m_screensize);
}

Framebuffer::~Framebuffer() {
    delete[] m_backbuffer;
    if (m_fb_ptr) munmap(m_fb_ptr, m_screensize);
    if (m_fd >= 0) close(m_fd);
}

void Framebuffer::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    // Fast clipping
    if ((x1 < 0 && x2 < 0) || (x1 >= (int)m_vinfo.xres && x2 >= (int)m_vinfo.xres)) return;
    if ((y1 < 0 && y2 < 0) || (y1 >= (int)m_vinfo.yres && y2 >= (int)m_vinfo.yres)) return;

    int32_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int32_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int32_t err = dx + dy;

    uint32_t stride = m_finfo.line_length;
    uint8_t* ptr = m_backbuffer + y1 * stride + (x1 << 2);
    int32_t step_x = sx << 2;
    int32_t step_y = sy * stride;

    while (true) {
        if (x1 >= 0 && x1 < (int)m_vinfo.xres && y1 >= 0 && y1 < (int)m_vinfo.yres) {
            *(uint32_t*)ptr = color;
        }
        
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; ptr += step_x; }
        if (e2 <= dx) { err += dx; y1 += sy; ptr += step_y; }
    }
}

void Framebuffer::clear(const Color& c) {
    if (c.r == 0 && c.g == 0 && c.b == 0) {
        memset(m_backbuffer, 0, m_screensize);
        return;
    }

    uint32_t color_val = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    uint32_t* buf = (uint32_t*)m_backbuffer;
    uint32_t count = m_screensize >> 2;
    for (uint32_t i = 0; i < count; ++i) buf[i] = color_val;
}

void Framebuffer::update() {
    if (m_fb_ptr && m_backbuffer) {
        // Try to wait for VSync to prevent tearing/lag
        uint32_t dummy = 0;
        ioctl(m_fd, FBIO_WAITFORVSYNC, &dummy);
        memcpy(m_fb_ptr, m_backbuffer, m_screensize);
    }
}

} // namespace core
