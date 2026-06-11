#include "APP.h"

void keyboard(bool enable) {
    static struct termios orig_termios;
    static bool is_enabled = false;

    if (enable) {
        if (is_enabled) return; // уже включено, не включаем повторно

        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        is_enabled = true;
    } else {
        if (!is_enabled) return; // уже выключено

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        is_enabled = false;
    }
}
