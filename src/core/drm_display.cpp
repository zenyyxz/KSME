#include "drm_display.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace core {

DrmDisplay::DrmDisplay(const char* device) : m_fd(-1), m_back_idx(0), m_active_fb_id(0) {
    m_fd = open(device, O_RDWR | O_CLOEXEC);
    if (m_fd < 0) {
        std::cerr << "Cannot open DRM device: " << device << std::endl;
        return;
    }

    if (!init_drm()) {
        close(m_fd);
        m_fd = -1;
    }
}

DrmDisplay::~DrmDisplay() {
    cleanup_drm();
}

bool DrmDisplay::init_drm() {
    // 1. Get resources
    struct drm_mode_card_res res = {};
    if (ioctl(m_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) return false;

    std::vector<uint32_t> conn_ids(res.count_connectors);
    res.connector_id_ptr = (uint64_t)conn_ids.data();
    if (ioctl(m_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) return false;

    // 2. Find a connected connector
    for (uint32_t i = 0; i < res.count_connectors; ++i) {
        struct drm_mode_get_connector conn = {};
        conn.connector_id = conn_ids[i];
        if (ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) continue;

        if (conn.connection == 1 && conn.count_modes > 0) {
            std::vector<struct drm_mode_modeinfo> modes(conn.count_modes);
            conn.modes_ptr = (uint64_t)modes.data();
            if (ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) continue;

            m_conn_id = conn.connector_id;
            m_mode = modes[0]; // Take the first (native) mode
            m_width = m_mode.hdisplay;
            m_height = m_mode.vdisplay;
            m_encoder_id = conn.encoder_id;
            break;
        }
    }

    if (m_conn_id == 0) return false;

    // 3. Find CRTC
    struct drm_mode_get_encoder enc = {};
    enc.encoder_id = m_encoder_id;
    if (ioctl(m_fd, DRM_IOCTL_MODE_GETENCODER, &enc) < 0) return false;
    m_crtc_id = enc.crtc_id;

    // 4. Create Buffers
    create_buffer(m_buffers[0]);
    create_buffer(m_buffers[1]);

    // 5. Initial Mode Set
    struct drm_mode_crtc set_crtc = {};
    set_crtc.crtc_id = m_crtc_id;
    set_crtc.fb_id = m_buffers[0].fb_id;
    set_crtc.set_connectors_ptr = (uint64_t)&m_conn_id;
    set_crtc.count_connectors = 1;
    set_crtc.mode = m_mode;
    set_crtc.mode_valid = 1;

    if (ioctl(m_fd, DRM_IOCTL_MODE_SETCRTC, &set_crtc) < 0) return false;
    m_active_fb_id = m_buffers[0].fb_id;
    m_back_idx = 1;

    return true;
}

void DrmDisplay::create_buffer(Buffer& buf) {
    struct drm_mode_create_dumb create = {};
    create.width = m_width;
    create.height = m_height;
    create.bpp = 32;
    ioctl(m_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
    buf.handle = create.handle;
    buf.stride = create.pitch;
    buf.size = create.size;

    struct drm_mode_fb_cmd add_fb = {};
    add_fb.width = m_width;
    add_fb.height = m_height;
    add_fb.pitch = buf.stride;
    add_fb.bpp = 32;
    add_fb.depth = 24;
    add_fb.handle = buf.handle;
    ioctl(m_fd, DRM_IOCTL_MODE_ADDFB, &add_fb);
    buf.fb_id = add_fb.fb_id;

    struct drm_mode_map_dumb map_dumb = {};
    map_dumb.handle = buf.handle;
    ioctl(m_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);

    buf.map = (uint8_t*)mmap(0, buf.size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, map_dumb.offset);
    memset(buf.map, 0, buf.size);
}

void DrmDisplay::update() {
    // Page Flip
    struct drm_mode_crtc_page_flip flip = {};
    flip.crtc_id = m_crtc_id;
    flip.fb_id = m_buffers[m_back_idx].fb_id;
    flip.flags = DRM_MODE_PAGE_FLIP_EVENT;
    
    if (ioctl(m_fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip) >= 0) {
        m_active_fb_id = m_buffers[m_back_idx].fb_id;
        m_back_idx = (m_back_idx + 1) % 2;

        // Wait for flip event (keeps the loop synced to refresh rate)
        struct drm_event ev;
        while (read(m_fd, &ev, sizeof(ev)) < 0);
    }
}

void DrmDisplay::clear(const Color& c) {
    uint32_t val = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    uint32_t* buf = (uint32_t*)m_buffers[m_back_idx].map;
    uint32_t count = m_buffers[m_back_idx].size >> 2;
    for (uint32_t i = 0; i < count; ++i) buf[i] = val;
}

void DrmDisplay::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    int32_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int32_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int32_t err = dx + dy;

    while (true) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void DrmDisplay::cleanup_drm() {
    if (m_fd < 0) return;
    for (int i = 0; i < 2; ++i) {
        if (m_buffers[i].map) munmap(m_buffers[i].map, m_buffers[i].size);
        if (m_buffers[i].fb_id) ioctl(m_fd, DRM_IOCTL_MODE_RMFB, &m_buffers[i].fb_id);
        if (m_buffers[i].handle) {
            struct drm_mode_destroy_dumb destroy = { m_buffers[i].handle };
            ioctl(m_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        }
    }
    close(m_fd);
    m_fd = -1;
}

} // namespace core
