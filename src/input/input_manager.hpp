#pragma once
#include <linux/input.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <poll.h>
#include <cstring>

namespace input {

class InputManager {
public:
    InputManager() : m_kbd_fd(-1), m_mouse_fd(-1) {
        std::memset(m_key_states, 0, sizeof(m_key_states));
        for (int i = 0; i < 32; ++i) {
            std::string path = "/dev/input/event" + std::to_string(i);
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd != -1) {
                unsigned long key_bitmask[KEY_CNT / (sizeof(unsigned long) * 8) + 1];
                std::memset(key_bitmask, 0, sizeof(key_bitmask));
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) >= 0) {
                    // Check if KEY_ESC bit is set as a heuristic for a keyboard
                    if ((key_bitmask[KEY_ESC / (sizeof(unsigned long) * 8)] >> (KEY_ESC % (sizeof(unsigned long) * 8))) & 1) {
                        if (m_kbd_fd == -1) m_kbd_fd = fd;
                        else close(fd);
                    } else {
                        close(fd);
                    }
                } else {
                    close(fd);
                }
            }
        }
    }

    ~InputManager() {
        if (m_kbd_fd != -1) close(m_kbd_fd);
        if (m_mouse_fd != -1) close(m_mouse_fd);
    }

    void update() {
        if (m_kbd_fd == -1) return;
        struct input_event ev;
        while (read(m_kbd_fd, &ev, sizeof(ev)) > 0) {
            if (ev.type == EV_KEY) {
                if (ev.code < KEY_CNT) {
                    m_key_states[ev.code] = ev.value;
                }
            }
        }
    }

    bool get_key_state(int key_code) const {
        if (key_code < 0 || key_code >= KEY_CNT) return false;
        return m_key_states[key_code] > 0;
    }

private:
    int m_kbd_fd;
    int m_mouse_fd;
    int m_key_states[KEY_CNT];
};

} // namespace input
