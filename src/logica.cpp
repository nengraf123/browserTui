#include "APP.h"

void APP::logica() {
  std::signal(SIGINT, signalHandler); // обработчик на нормальный выход
  WIDTH_TERMINAL=terminal_size_WIDTH();
  HEIGHT_TERMINAL=terminal_size_HEIGHT();
  monitor = monitor_L();
  page = page_L();
};

std::string APP::monitor_L(){
  int monitor_size = (WIDTH_TERMINAL * HEIGHT_TERMINAL);
  std::string monitor(monitor_size, ' ');
  return monitor;
};
std::string APP::page_L(){
  int page_size = ((HEIGHT_TERMINAL-2) * WIDTH_TERMINAL); 
  std::string page(page_size, ' ');
  return page;
};

int APP::terminal_size_HEIGHT() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_row;
    }
    return -1;
}

int APP::terminal_size_WIDTH() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return -1;
}

void APP::keyboard(bool enable) {
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

APP* g_app_instance = nullptr;
void signalHandler(int signum) {
    if (g_app_instance) g_app_instance->keyboard(false);
    std::cout << "\033[?25h" << std::flush;
    std::cout << "\033[0m" << std::flush;
    exit(signum);
}

void APP::monitor_draw() {
  std::cout << "\033[H"; // перенести курсор в верхний левый угол
  std::cout << "\033[48;2;0;0;175m" << monitor << "\033[0m" << std::flush; monitor.clear();
  std::cout << "\033[48;2;111;111;111m" << page << "\033[0m" << std::flush; page.clear();
};
void APP::page_draw() {
  std::cout << "\033[H"; // перенести курсор в верхний левый угол
  std::cout << "\033[48;2;111;111;111m" << page << "\033[0m" << std::flush; page.clear();
};
