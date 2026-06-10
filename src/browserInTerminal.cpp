#include "APP.h"
#include <stdexcept>

#define $sss printf("\n");
#define ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА printf("\033[H");
#define ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_СТРОЧКИ printf("\033[G");
// 1. Обычная клавиша (Enter, мелкие буквы, знаки)
#define KEY(ch) (n == 1 && input_buf[0] == (ch))
// 2. Shift + Буква (Просто проверяем заглавную букву)
#define KEY_SHIFT(ch) (n == 1 && input_buf[0] == (ch))
// 3. Alt + Буква (Два байта: Escape + буква в любом регистре)
#define KEY_ALT(ch) (n == 2 && input_buf[0] == '\33' && input_buf[1] == (ch))
// 4. Ctrl + Буква (Один байт. Формула превращает 't' или 'T' в код Ctrl-сочетания)
#define KEY_CTRL(ch) (n == 1 && input_buf[0] == ((ch) & 0x1F))

#define КУРСОР_ВПРАВО(n)   std::cout << "\033[" << n << "C" << std::flush
#define КУРСОР_ВЛЕВО(n)    std::cout << "\033[" << n << "D" << std::flush
#define КУРСОР_ВВЕРХ(n)    std::cout << "\033[" << n << "A" << std::flush
#define КУРСОР_ВНИЗ(n)    std::cout << "\033[" << n << "B" << std::flush
#define КУРСОР_НА(row, col) std::cout << "\033[" << row << ";" << col << "H" << std::flush


void terminal_size(int &width, int &height) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        width = w.ws_col;
        height = w.ws_row;
    }
}
#define ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА terminal_size(WIDTH_TERMINAL, HEIGHT_TERMINAL);

struct termios orig_termios;

template <typename T>
double percent(T percent, T total) {return (static_cast<double>(total) * percent) / 100.0;}

// обработчик на нормальный выход
void signalHandler(int signum) {
    keyboard(false);
    std::cout << "\033[?25h" << std::flush;
    std::cout << "\033[0m" << std::flush;
    exit(signum);
}


// мне нужно не по символьно делать холст а делать все в памяти а потом просто вывести это
// мне нужно что бы каждый символ не занимал так много, как я могу это реализовать в своем стиле?
// мне нужно научится изменять пиксели на уже другой реализованной лигике

// Сохраняем базовые цвета в переменные для удобства
std::string WHITE_BG = "\033[48;2;255;255;255m";
std::string BLUE_BG = "\033[48;2;0;18;210m";
std::string GREEN_BG = "\033[48;2;0;255;0m";
std::string RED_BG = "\033[48;2;255;0;0m";


int APP::browserInTerminal() {
  // std::cout << "\033[?25l" << std::flush; // 1. СКРЫВАЕМ КУРСОР ПЕРЕД ЦИКЛОМ
  keyboard(true);  // включаем ввод с клавиатуры
  std::signal(SIGINT, signalHandler); // обработчик на нормальный выход
  int WIDTH_TERMINAL;
  int HEIGHT_TERMINAL;
  int monitor_size;
  std::string monitor;
  struct Tab {
    int id;
    std::string title = "Новая вкладка"; // Значение по умолчанию
    std::string url;
    bool is_active;
    bool is_pinned;
    bool is_loading;
    bool is_muted;
  };
  int tab_count = 1;
  int tab_focus = tab_count;

  bool tab_active = false;
  while (1) {
    // для клавиш
    char input_buf[3] = {0}; int n = read(STDIN_FILENO, input_buf, sizeof(input_buf));
    ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
    ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА

    // считаем размер экрана и подставляем туда пробелы
    monitor_size = (HEIGHT_TERMINAL * WIDTH_TERMINAL) ;
    monitor.assign(monitor_size, ' ');

    std::cout << "\033[48;2;0;0;175m" << monitor << "\033[0m" << std::flush;

    ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
    // считаем ширину таба
    auto size_width_tab = [&]() -> int {
        int available = (int)(percent(90, WIDTH_TERMINAL)); // 90% ширины на все табы
        int tab_width = available / tab_count;              // делим поровну
        int max_tab_width = (int)percent(15, WIDTH_TERMINAL); // максимум одного таба
        return std::min(tab_width, max_tab_width);          // берём меньшее
    };
    // ложим туда символы
    std::string width_tab(size_width_tab(), '0');


    if (KEY('t')) {
      tab_count++;
      tab_focus = tab_count;
    }
    if (KEY('w') && tab_count > 1) {
    tab_count--;
    if (tab_focus >= tab_count) tab_focus = tab_count; // не выходим за границу
    }
    for (int i = 1; i < tab_count+1; i++) {
      if (i == tab_focus) {
        // Активная вкладка — белая
        std::cout << "\033[4m\033[48;2;205;205;205m\033[30m" << width_tab << "\u2717\033[0m" << std::flush;
      } else {
        // Неактивная вкладка — чёрная
        std::cout << "\033[4m\033[48;2;50;50;50m\033[38;2;255;255;255m" << width_tab << "\u2717\033[0m" << std::flush;
      }
    }

    КУРСОР_НА(HEIGHT_TERMINAL, 1);
    std::cout << size_width_tab() << std::flush;

    monitor.clear();
    usleep(1000000 / 30);
  };
  keyboard(false);
};
// std::cout << "\033[4m" << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << "\u2717" << "\033[0m" << std::flush;
// #define m_ "\033[4m"
// #define mx "\u2717"
// screen = screen + "\033[38;2;0;180;210;48;2;0;18;210m \033[0m";
// fflush(stdout);
// std::cout << m_ << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << mx << "\033[0m" << std::flush;
// std::cout << m_ << "\033[48;2;255;255;255m" << "\033[30m" << width_tab << mx << "\033[0m" << std::flush;
// if (tab_active) {
//   std::cout << m_ << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << mx << "\033[0m" << std::flush;
// } else {
//   std::cout << m_ << "\033[48;2;255;255;255m" << "\033[30m" << width_tab << mx << "\033[0m" << std::flush;
// }
