#include "framebuffer.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
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

void Framebuffer::put_pixel(uint32_t x, uint32_t y, const Color& c) {
    if (x >= m_vinfo.xres || y >= m_vinfo.yres) return;

    // Direct offset calculation - assuming 32bpp for now as it's standard on modern fb
    // We can add 16bpp branch back if needed, but keeping it lean for now.
    uint32_t* pix = (uint32_t*)(m_backbuffer + y * m_finfo.line_length + x * 4);
    *pix = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
}

void Framebuffer::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const Color& c) {
    int32_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int32_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int32_t err = dx + dy;

    while (true) {
        put_pixel(x1, y1, c);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void Framebuffer::clear(const Color& c) {
    // If it's black, memset is lightning fast
    if (c.r == 0 && c.g == 0 && c.b == 0) {
        memset(m_backbuffer, 0, m_screensize);
        return;
    }

    // Otherwise, fill the first line and then use memcpy for the rest
    uint32_t color_val = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    uint32_t* line = (uint32_t*)m_backbuffer;
    for (uint32_t x = 0; x < m_vinfo.xres; ++x) line[x] = color_val;

    for (uint32_t y = 1; y < m_vinfo.yres; ++y) {
        memcpy(m_backbuffer + y * m_finfo.line_length, m_backbuffer, m_finfo.line_length);
    }
}

void Framebuffer::update() {
    if (m_fb_ptr && m_backbuffer) {
        memcpy(m_fb_ptr, m_backbuffer, m_screensize);
    }
}

} // namespace core
