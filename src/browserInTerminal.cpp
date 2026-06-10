#include "APP.h"
#include <ostream>

#define $sss printf("\n");
#define ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА printf("\033[H");
#define ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_СТРОЧКИ printf("\033[G");

void terminal_size(int &width, int &height) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        width = w.ws_col;
        height = w.ws_row;
    }
}
#define ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА terminal_size(WIDTH_TERMINAL, HEIGHT_TERMINAL);

template <typename T>
double percent(T percent, T total) {return (static_cast<double>(total) * percent) / 100.0;}



// мне нужно не по символьно делать холст а делать все в памяти а потом просто вывести это
// мне нужно что бы каждый символ не занимал так много, как я могу это реализовать в своем стиле?
// мне нужно научится изменять пиксели на уже другой реализованной лигике

// Сохраняем базовые цвета в переменные для удобства
std::string WHITE_BG = "\033[48;2;255;255;255m";
std::string BLUE_BG = "\033[48;2;0;18;210m";
std::string GREEN_BG = "\033[48;2;0;255;0m";
std::string RED_BG = "\033[48;2;255;0;0m";

int APP::browserInTerminal() {
  int WIDTH_TERMINAL;
  int HEIGHT_TERMINAL;
  int monitor_size;
  std::string monitor;

  // 1. СКРЫВАЕМ КУРСОР ПЕРЕД ЦИКЛОМ
  std::cout << "\033[?25l" << std::flush;
  while (1) {
  ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
  ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА

  monitor_size = (HEIGHT_TERMINAL * WIDTH_TERMINAL) ;
  monitor.assign(monitor_size, ' ');

  std::cout << "\033[48;2;0;0;175m" << monitor << "\033[0m" << std::flush;

  ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
  double percent_width_tab = percent(15, WIDTH_TERMINAL);
  std::string width_tab(percent_width_tab, ' ');
  // std::cout << "\033[4m" << "\033[48;2;111;111;111m" << "\033[30m" << width_tab << "\u2717" << "\033[0m" << std::flush;
  std::cout << "\033[4m" << "\033[48;2;111;111;111m" << "\033[38;2;144;144;144m" << width_tab << "\u2717" << "\033[0m" << std::flush;


  monitor.clear();
    usleep(16666);
  };
};
// screen = screen + "\033[38;2;0;180;210;48;2;0;18;210m \033[0m";
// fflush(stdout);
