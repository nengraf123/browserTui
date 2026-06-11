#define p printf
#define dbl double
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
// 1. Стрелка ВВЕРХ (Escape + '[' + 'A')
#define KEY_UP    (n == 3 && input_buf[0] == '\33' && input_buf[1] == '[' && input_buf[2] == 'A')
// 2. Стрелка ВНИЗ (Escape + '[' + 'B')
#define KEY_DOWN  (n == 3 && input_buf[0] == '\33' && input_buf[1] == '[' && input_buf[2] == 'B')
// 3. Стрелка ВПРАВО (Escape + '[' + 'C')
#define KEY_RIGHT (n == 3 && input_buf[0] == '\33' && input_buf[1] == '[' && input_buf[2] == 'C')
// 4. Стрелка ВЛЕВО (Escape + '[' + 'D')
#define KEY_LEFT  (n == 3 && input_buf[0] == '\33' && input_buf[1] == '[' && input_buf[2] == 'D')
#define КУРСОР_ВПРАВО(n)   std::cout << "\033[" << n << "C" << std::flush
#define КУРСОР_ВЛЕВО(n)    std::cout << "\033[" << n << "D" << std::flush
#define КУРСОР_ВВЕРХ(n)    std::cout << "\033[" << n << "A" << std::flush
#define КУРСОР_ВНИЗ(n)    std::cout << "\033[" << n << "B" << std::flush
#define КУРСОР_НА(row, col) std::cout << "\033[" << row << ";" << col << "H" << std::flush
#define CURSOR_TO(row, col) std::cout << "\033[" << row << ";" << col << "H" << std::flush

