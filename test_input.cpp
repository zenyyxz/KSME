#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: sudo ./test_input /dev/input/eventX" << std::endl;
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    std::cout << "Testing input on " << argv[1] << ". Press keys (Ctrl+C to exit)..." << std::endl;

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY) {
            std::cout << "KEY: code=" << ev.code << " value=" << ev.value 
                      << (ev.value == 1 ? " [PRESS]" : (ev.value == 0 ? " [RELEASE]" : " [REPEAT]")) 
                      << std::endl;
        }
    }

    close(fd);
    return 0;
}
