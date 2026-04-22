#pragma once
#include <linux/input.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <poll.h>
#include <cstring>

#ifndef KEY_1
#define KEY_1 2
#endif
#ifndef KEY_2
#define KEY_2 3
#endif
#ifndef KEY_3
#define KEY_3 4
#endif

namespace input {

class InputManager {
public:
    InputManager() : m_kbd_fd(-1), m_mouse_fd(-1) {
        std::memset(m_key_states, 0, sizeof(m_key_states));
        m_mouse_x = 0; m_mouse_y = 0;
        
        for (int i = 0; i < 32; ++i) {
            std::string path = "/dev/input/event" + std::to_string(i);
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd != -1) {
                unsigned long key_bitmask[KEY_CNT / (sizeof(unsigned long) * 8) + 1];
                unsigned long rel_bitmask[REL_CNT / (sizeof(unsigned long) * 8) + 1];
                std::memset(key_bitmask, 0, sizeof(key_bitmask));
                std::memset(rel_bitmask, 0, sizeof(rel_bitmask));
                
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) >= 0) {
                    if ((key_bitmask[KEY_ESC / (sizeof(unsigned long) * 8)] >> (KEY_ESC % (sizeof(unsigned long) * 8))) & 1) {
                        if (m_kbd_fd == -1) m_kbd_fd = fd;
                        else close(fd);
                        continue;
                    }
                }
                
                if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bitmask)), rel_bitmask) >= 0) {
                    if ((rel_bitmask[REL_X / (sizeof(unsigned long) * 8)] >> (REL_X % (sizeof(unsigned long) * 8))) & 1) {
                        if (m_mouse_fd == -1) m_mouse_fd = fd;
                        else close(fd);
                        continue;
                    }
                }
                close(fd);
            }
        }
    }

    ~InputManager() {
        if (m_kbd_fd != -1) close(m_kbd_fd);
        if (m_mouse_fd != -1) close(m_mouse_fd);
    }

    void update() {
        m_mouse_x = 0; m_mouse_y = 0;
        
        if (m_kbd_fd != -1) {
            struct input_event ev;
            while (read(m_kbd_fd, &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_KEY && ev.code < KEY_CNT) {
                    m_key_states[ev.code] = ev.value;
                }
            }
        }

        if (m_mouse_fd != -1) {
            struct input_event ev;
            while (read(m_mouse_fd, &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X) m_mouse_x += ev.value;
                    if (ev.code == REL_Y) m_mouse_y += ev.value;
                }
            }
        }
    }

    bool get_key_state(int key_code) const {
        if (key_code < 0 || key_code >= KEY_CNT) return false;
        return m_key_states[key_code] > 0;
    }

    int get_mouse_dx() const { return m_mouse_x; }
    int get_mouse_dy() const { return m_mouse_y; }

private:
    int m_kbd_fd;
    int m_mouse_fd;
    int m_key_states[KEY_CNT];
    int m_mouse_x, m_mouse_y;
};

} // namespace input
