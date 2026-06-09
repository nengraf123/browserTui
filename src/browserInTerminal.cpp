#include "APP.h"

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

void draw_pixel(std::string &screen, int width, int x, int y, const std::string &color_ansi, const std::string &base_bg_ansi) {
    // 1. Считаем «чистый» индекс, как если бы строка состояла только из пробелов
    size_t target_pure_index = (y * width) + y + x;

    // 2. Считаем реальный индекс в строке с учетом уже добавленных ранее ANSI-кодов
    size_t real_index = 0;
    size_t pure_chars_counted = 0;

    while (real_index < screen.length() && pure_chars_counted < target_pure_index) {
        // Если наткнулись на начало ANSI-последовательности, просто перепрыгиваем её всю
        if (screen[real_index] == '\033') {
            while (real_index < screen.length() && screen[real_index] != 'm') {
                real_index++;
            }
            real_index++; // перешагиваем саму букву 'm'
        } else {
            // Если это обычный пробел или '\n', учитываем его в счетчике чистых символов
            pure_chars_counted++;
            real_index++;
        }
    }

    // Проверяем, что не вылетели за физический размер строки
    if (real_index >= screen.length()) return;

    // 3. Формируем заплатку: включаем цвет -> рисуем пробел -> возвращаем цвет по цепочке дальше
    std::string pixel_patch = color_ansi + " " + base_bg_ansi;

    // 4. Заменяем ровно 1 чистый пробел по правильному реальному индексу
    screen.replace(real_index, 1, pixel_patch);
}

// мне нужно не по символьно делать холст а делать все в памяти а потом просто вывести это
// мне нужно что бы каждый символ не занимал так много, как я могу это реализовать в своем стиле?

int APP::browserInTerminal() {
  int WIDTH_TERMINAL;
  int HEIGHT_TERMINAL;
  std::string screen = "";

  // Сохраняем базовые цвета в переменные для удобства
  std::string WHITE_BG = "\033[48;2;255;255;255m";
  std::string BLUE_BG = "\033[48;2;0;18;210m";
  std::string GREEN_BG = "\033[48;2;0;255;0m";
  std::string RED_BG = "\033[48;2;255;0;0m";





  while (1) {
    ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
      ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА
      for (int j=0; j<HEIGHT_TERMINAL;j++) {
        for (int i=0; i<WIDTH_TERMINAL;i++) {
          screen = screen + " ";
        };
        if (j < HEIGHT_TERMINAL - 2) screen += "\n"; // Добавляем перенос только если это НЕ самая последняя строка холста
      };
    draw_pixel(screen, WIDTH_TERMINAL, 0, 0, WHITE_BG, WHITE_BG); 
    draw_pixel(screen, WIDTH_TERMINAL, 0, 12, BLUE_BG, BLUE_BG); 
    draw_pixel(screen, WIDTH_TERMINAL, 0, 27, RED_BG, RED_BG); 
    std::cout << "\033[48;2;0;18;210m" << screen << "\033[0m" << std::flush;
    screen = "";
    usleep(16666);
  };
};
// screen = screen + "\033[38;2;0;180;210;48;2;0;18;210m \033[0m";
// fflush(stdout);
