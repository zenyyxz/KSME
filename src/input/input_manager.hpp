/*
 * ASYNCHRONOUS INPUT SUBSYSTEM
 * Interfaces with Linux evdev (/dev/input/event*) for low-latency event processing.
 * Supports multiple concurrent input devices (Multi-KBD/Mouse).
 */

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
    InputManager() : m_mouse_fd(-1) {
        /*
         * HARDWARE DISCOVERY
         * Iterates through the evdev subsystem to identify suitable HID devices.
         * Filtered by capability bitmasks (EVIOCGBIT) to distinguish between
         * keyboards (KEY_ESC presence) and mice (REL_X/REL_Y presence).
         */
        std::memset(m_key_states, 0, sizeof(m_key_states));
        m_mouse_x = 0; m_mouse_y = 0; m_last_char = 0;
        
        for (int i = 0; i < 32; ++i) {
            std::string path = "/dev/input/event" + std::to_string(i);
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd != -1) {
                unsigned long key_bitmask[KEY_CNT / (sizeof(unsigned long) * 8) + 1];
                unsigned long rel_bitmask[REL_CNT / (sizeof(unsigned long) * 8) + 1];
                std::memset(key_bitmask, 0, sizeof(key_bitmask));
                std::memset(rel_bitmask, 0, sizeof(rel_bitmask));
                
                bool is_kbd = false;
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) >= 0) {
                    if ((key_bitmask[KEY_ESC / (sizeof(unsigned long) * 8)] >> (KEY_ESC % (sizeof(unsigned long) * 8))) & 1) {
                        is_kbd = true;
                    }
                }
                
                bool is_mouse = false;
                if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bitmask)), rel_bitmask) >= 0) {
                    if ((rel_bitmask[REL_X / (sizeof(unsigned long) * 8)] >> (REL_X % (sizeof(unsigned long) * 8))) & 1) {
                        is_mouse = true;
                    }
                }

                if (is_kbd) {
                    m_kbd_fds.push_back(fd);
                } else if (is_mouse && m_mouse_fd == -1) {
                    m_mouse_fd = fd;
                } else {
                    close(fd);
                }
            }
        }
    }

    ~InputManager() {
        for (int fd : m_kbd_fds) close(fd);
        if (m_mouse_fd != -1) close(m_mouse_fd);
    }

    void update() {
        /*
         * ASYNCHRONOUS EVENT POLLING
         * Performs non-blocking reads on all registered hardware file descriptors.
         * Accumulates delta-movements and latches key states for frame-synchronous retrieval.
         */
        m_mouse_x = 0; m_mouse_y = 0;
        
        for (int fd : m_kbd_fds) {
            struct input_event ev;
            while (read(fd, &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_KEY && ev.code < KEY_CNT) {
                    m_key_states[ev.code] = ev.value;
                    if (ev.value == 1) { // Press
                        m_last_raw_code = ev.code;
                        m_last_shift_state = (m_key_states[KEY_LEFTSHIFT] > 0) || (m_key_states[KEY_RIGHTSHIFT] > 0);
                        char c = keycode_to_char(ev.code);
                        if (c != 0) m_last_char = c;
                    }
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
    int get_last_raw_code() const { return m_last_raw_code; }
    bool get_last_shift_state() const { return m_last_shift_state; }
    
    char get_last_char() { 
        char c = m_last_char;
        m_last_char = 0; 
        return c; 
    }

private:
    /*
     * CHARACTER TRANSLATION LAYER
     * Maps raw Linux input event codes to ASCII representations.
     * Samples modifiers (Shift) at the point-of-interest for the parser buffer.
     */
    char keycode_to_char(int code) {
        bool shift = (m_key_states[KEY_LEFTSHIFT] > 0) || (m_key_states[KEY_RIGHTSHIFT] > 0);
        switch (code) {
            case KEY_A: return shift ? 'A' : 'a'; case KEY_B: return shift ? 'B' : 'b';
            case KEY_C: return shift ? 'C' : 'c'; case KEY_D: return shift ? 'D' : 'd';
            case KEY_E: return shift ? 'E' : 'e'; case KEY_F: return shift ? 'F' : 'f';
            case KEY_G: return shift ? 'G' : 'g'; case KEY_H: return shift ? 'H' : 'h';
            case KEY_I: return shift ? 'I' : 'i'; case KEY_J: return shift ? 'J' : 'j';
            case KEY_K: return shift ? 'K' : 'k'; case KEY_L: return shift ? 'L' : 'l';
            case KEY_M: return shift ? 'M' : 'm'; case KEY_N: return shift ? 'N' : 'n';
            case KEY_O: return shift ? 'O' : 'o'; case KEY_P: return shift ? 'P' : 'p';
            case KEY_Q: return shift ? 'Q' : 'q'; case KEY_R: return shift ? 'R' : 'r';
            case KEY_S: return shift ? 'S' : 's'; case KEY_T: return shift ? 'T' : 't';
            case KEY_U: return shift ? 'U' : 'u'; case KEY_V: return shift ? 'V' : 'v';
            case KEY_W: return shift ? 'W' : 'w'; case KEY_X: return shift ? 'X' : 'x';
            case KEY_Y: return shift ? 'Y' : 'y'; case KEY_Z: return shift ? 'Z' : 'z';
            
            case KEY_1: return shift ? '!' : '1'; case KEY_2: return shift ? '@' : '2';
            case KEY_3: return shift ? '#' : '3'; case KEY_4: return shift ? '$' : '4';
            case KEY_5: return shift ? '%' : '5'; case KEY_6: return shift ? '^' : '6';
            case KEY_7: return shift ? '&' : '7'; case KEY_8: return shift ? '*' : '8';
            case KEY_9: return shift ? '(' : '9'; case KEY_0: return shift ? ')' : '0';

            case KEY_SPACE: return ' ';
            case KEY_MINUS: return shift ? '_' : '-';
            case KEY_EQUAL: return shift ? '+' : '=';
            case KEY_LEFTBRACE: return shift ? '{' : '[';
            case KEY_RIGHTBRACE: return shift ? '}' : ']';
            case KEY_SEMICOLON: return shift ? ':' : ';';
            case KEY_APOSTROPHE: return shift ? '"' : '\'';
            case KEY_GRAVE: return shift ? '~' : '`';
            case KEY_BACKSLASH: return shift ? '|' : '\\';
            case KEY_COMMA: return shift ? '<' : ',';
            case KEY_DOT: return shift ? '>' : '.';
            case KEY_SLASH: return shift ? '?' : '/';
            case KEY_BACKSPACE: return '\b';
            case KEY_ENTER: return '\n';
            default: return 0;
        }
    }

    std::vector<int> m_kbd_fds;
    int m_mouse_fd;
    int m_key_states[KEY_CNT];
    int m_mouse_x, m_mouse_y;
    int m_last_raw_code = 0;
    bool m_last_shift_state = false;
    char m_last_char;
};

} // namespace input
