/*
 * DRM/KMS Display Backend
 * Handles direct hardware access for flicker-free rendering without a display server.
 */

#include "drm_display.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <errno.h>
#include <poll.h>

namespace core {

DrmDisplay::DrmDisplay(const char* device) : m_fd(-1), m_back_idx(0), m_active_fb_id(0) {
    m_fd = open(device, O_RDWR | O_CLOEXEC);
    if (m_fd < 0) {
        std::cerr << "Error: Cannot open DRM device " << device << " (" << strerror(errno) << ")" << std::endl;
        return;
    }

    if (!init_drm()) {
        cleanup_drm();
    }
}

DrmDisplay::~DrmDisplay() {
    cleanup_drm();
}

bool DrmDisplay::init_drm() {
    std::cerr << "Initializing DRM..." << std::endl;
    // 1. Resource Discovery
    // Query the kernel for available connectors (physical ports), encoders, and CRTCs (scanout engines).
    struct drm_mode_card_res res = {};
    if (ioctl(m_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        std::cerr << "DRM_IOCTL_MODE_GETRESOURCES failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (res.count_connectors == 0) {
        std::cerr << "No connectors found on this DRM device." << std::endl;
        return false;
    }

    std::vector<uint32_t> conn_ids(res.count_connectors);
    std::vector<uint32_t> fb_ids(res.count_fbs);
    std::vector<uint32_t> crtc_ids(res.count_crtcs);
    std::vector<uint32_t> enc_ids(res.count_encoders);

    res.connector_id_ptr = (uint64_t)conn_ids.data();
    res.fb_id_ptr = (uint64_t)fb_ids.data();
    res.crtc_id_ptr = (uint64_t)crtc_ids.data();
    res.encoder_id_ptr = (uint64_t)enc_ids.data();

    if (ioctl(m_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        std::cerr << "DRM_IOCTL_MODE_GETRESOURCES (2nd) failed: " << strerror(errno) << std::endl;
        return false;
    }

    // 2. Connector Selection
    // Locate the first physical display that is connected and has valid video modes.
    bool found_conn = false;
    for (uint32_t i = 0; i < res.count_connectors; ++i) {
        struct drm_mode_get_connector conn = {};
        conn.connector_id = conn_ids[i];
        if (ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) {
            std::cerr << "DRM_IOCTL_MODE_GETCONNECTOR failed for ID " << conn_ids[i] << ": " << strerror(errno) << std::endl;
            continue;
        }

        if (conn.connection == 1 && conn.count_modes > 0) {
            std::vector<struct drm_mode_modeinfo> modes(conn.count_modes);
            std::vector<uint32_t> encoders(conn.count_encoders);
            std::vector<uint32_t> props(conn.count_props);
            std::vector<uint64_t> prop_values(conn.count_props);

            conn.modes_ptr = (uint64_t)modes.data();
            conn.encoders_ptr = (uint64_t)encoders.data();
            conn.props_ptr = (uint64_t)props.data();
            conn.prop_values_ptr = (uint64_t)prop_values.data();

            if (ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) {
                std::cerr << "DRM_IOCTL_MODE_GETCONNECTOR (2nd) failed: " << strerror(errno) << std::endl;
                continue;
            }

            m_conn_id = conn.connector_id;
            m_mode = modes[0]; // Take the first (native) mode
            m_width = m_mode.hdisplay;
            m_height = m_mode.vdisplay;
            m_encoder_id = conn.encoder_id;
            found_conn = true;
            break;
        }
    }

    if (!found_conn) {
        std::cerr << "No connected DRM connector with valid modes found." << std::endl;
        return false;
    }

    // 3. Encoder and CRTC Routing
    // Map the selected connector to a hardware pipeline (CRTC).
    if (m_encoder_id == 0) {
        // If no encoder is attached, try the first one supported by the connector
        struct drm_mode_get_connector conn = {};
        conn.connector_id = m_conn_id;
        ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);
        std::vector<uint32_t> encs(conn.count_encoders);
        conn.encoders_ptr = (uint64_t)encs.data();
        conn.connector_id = m_conn_id;
        ioctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);
        if (conn.count_encoders > 0) {
            m_encoder_id = encs[0];
        } else {
            std::cerr << "Connector has no encoders." << std::endl;
            return false;
        }
    }

    struct drm_mode_get_encoder enc = {};
    enc.encoder_id = m_encoder_id;
    if (ioctl(m_fd, DRM_IOCTL_MODE_GETENCODER, &enc) < 0) {
        std::cerr << "DRM_IOCTL_MODE_GETENCODER failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Pick a CRTC. If encoder has none, pick the first available from card resources
    m_crtc_id = enc.crtc_id ? enc.crtc_id : (res.count_crtcs > 0 ? crtc_ids[0] : 0);
    if (m_crtc_id == 0) {
        std::cerr << "No valid CRTC found for encoder." << std::endl;
        return false;
    }

    // 4. Memory-Mapped Buffer Creation
    // Allocate dual dumb buffers for software double-buffering.
    create_buffer(m_buffers[0]);
    create_buffer(m_buffers[1]);
    if (m_buffers[0].map == nullptr || m_buffers[1].map == nullptr) {
        std::cerr << "Failed to create/map buffers." << std::endl;
        return false;
    }

    // 5. Acquisition of DRM Master
    // Request exclusive control of the graphics hardware.
    if (ioctl(m_fd, DRM_IOCTL_SET_MASTER, 0) < 0) {
        std::cerr << "DRM_IOCTL_SET_MASTER failed: " << strerror(errno) << " (Check if another display manager is running)" << std::endl;
    }

    // 6. Initial Mode Set
    // Commit the hardware configuration and initialize the primary plane.
    struct drm_mode_crtc set_crtc = {};
    set_crtc.crtc_id = m_crtc_id;
    set_crtc.fb_id = m_buffers[0].fb_id;
    set_crtc.set_connectors_ptr = (uint64_t)&m_conn_id;
    set_crtc.count_connectors = 1;
    set_crtc.mode = m_mode;
    set_crtc.mode_valid = 1;

    if (ioctl(m_fd, DRM_IOCTL_MODE_SETCRTC, &set_crtc) < 0) {
        std::cerr << "DRM_IOCTL_MODE_SETCRTC failed: " << strerror(errno) << std::endl;
        return false;
    }
    m_active_fb_id = m_buffers[0].fb_id;
    m_back_idx = 1;

    return true;
}

void DrmDisplay::create_buffer(Buffer& buf) {
    struct drm_mode_create_dumb create = {};
    create.width = m_width;
    create.height = m_height;
    create.bpp = 32;
    if (ioctl(m_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        std::cerr << "DRM_IOCTL_MODE_CREATE_DUMB failed: " << strerror(errno) << std::endl;
        return;
    }
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
    if (ioctl(m_fd, DRM_IOCTL_MODE_ADDFB, &add_fb) < 0) {
        std::cerr << "DRM_IOCTL_MODE_ADDFB failed: " << strerror(errno) << std::endl;
        return;
    }
    buf.fb_id = add_fb.fb_id;

    struct drm_mode_map_dumb map_dumb = {};
    map_dumb.handle = buf.handle;
    if (ioctl(m_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) < 0) {
        std::cerr << "DRM_IOCTL_MODE_MAP_DUMB failed: " << strerror(errno) << std::endl;
        return;
    }

    buf.map = (uint8_t*)mmap(0, buf.size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, map_dumb.offset);
    if (buf.map == MAP_FAILED) {
        std::cerr << "mmap failed: " << strerror(errno) << std::endl;
        buf.map = nullptr;
        return;
    }
    memset(buf.map, 0, buf.size);
}

void DrmDisplay::update() {
    /*
     * Hardware Page Flipping
     * Schedules a flip to the backbuffer on the next VBlank interval.
     * Synchronizes the engine to the physical refresh rate of the monitor.
     */
    struct drm_mode_crtc_page_flip flip = {};
    flip.crtc_id = m_crtc_id;
    flip.fb_id = m_buffers[m_back_idx].fb_id;
    flip.flags = DRM_MODE_PAGE_FLIP_EVENT;
    
    if (ioctl(m_fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip) >= 0) {
        m_active_fb_id = m_buffers[m_back_idx].fb_id;
        m_back_idx = (m_back_idx + 1) % 2;

        // Wait for the flip event with a 100ms safety timeout to prevent kernel hangs
        struct pollfd pfd = { .fd = m_fd, .events = POLLIN, .revents = 0 };
        if (poll(&pfd, 1, 100) > 0) { 
            uint8_t buffer[1024];
            int len;
            while ((len = read(m_fd, buffer, sizeof(buffer))) < 0) {
                if (errno != EINTR) break; 
            }
        }
    } else {
        // Fallback: Immediate mode set if atomic page flipping is unavailable
        struct drm_mode_crtc set_crtc = {};
        set_crtc.crtc_id = m_crtc_id;
        set_crtc.fb_id = m_buffers[m_back_idx].fb_id;
        set_crtc.set_connectors_ptr = (uint64_t)&m_conn_id;
        set_crtc.count_connectors = 1;
        set_crtc.mode = m_mode;
        set_crtc.mode_valid = 1;
        
        if (ioctl(m_fd, DRM_IOCTL_MODE_SETCRTC, &set_crtc) >= 0) {
            m_active_fb_id = m_buffers[m_back_idx].fb_id;
            m_back_idx = (m_back_idx + 1) % 2;
        }

        // Throttling to prevent CPU saturation in fallback mode
        usleep(1000); 
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
