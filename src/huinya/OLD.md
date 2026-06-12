        #include "APP.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <thread>
#include <chrono>

static int connect_local(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }
    return sock;
}

static std::string read_all(int sock, int timeout_ms) {
    std::string out;
    char buf[65536];
    timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (true) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
        out.append(buf, n);
    }
    return out;
}

static void ws_send_text(int sock, const std::string& payload) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    size_t len = payload.size();
    if (len < 126) {
        frame.push_back(0x80 | (uint8_t)len);
    } else if (len < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; i--) frame.push_back((len >> (8*i)) & 0xFF);
    }
    uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78};
    frame.insert(frame.end(), mask, mask+4);
    for (size_t i = 0; i < len; i++)
        frame.push_back(payload[i] ^ mask[i % 4]);
    send(sock, frame.data(), frame.size(), 0);
}

// читает один фрейм (может быть с маской или без)
static std::string ws_recv_frame(int sock, int timeout_ms) {
    timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t hdr[2];
    ssize_t r = recv(sock, hdr, 2, 0);
    if (r <= 0) return "";

    bool masked = hdr[1] & 0x80;
    uint64_t len = hdr[1] & 0x7F;
    if (len == 126) {
        uint8_t ext[2];
        recv(sock, ext, 2, 0);
        len = (ext[0] << 8) | ext[1];
    } else if (len == 127) {
        uint8_t ext[8];
        recv(sock, ext, 8, 0);
        len = 0;
        for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
    }

    uint8_t maskkey[4] = {0,0,0,0};
    if (masked) recv(sock, maskkey, 4, 0);

    std::string payload(len, 0);
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(sock, payload.data() + got, len - got, 0);
        if (n <= 0) break;
        got += n;
    }
    if (masked) {
        for (size_t i = 0; i < payload.size(); i++)
            payload[i] ^= maskkey[i % 4];
    }
    return payload;
}

// найти webSocketDebuggerUrl для записи с "type": "page"
static std::string find_page_ws_url(const std::string& json) {
    size_t pos = 0;
    while (true) {
        size_t type_pos = json.find("\"type\": \"page\"", pos);
        if (type_pos == std::string::npos) {
            type_pos = json.find("\"type\":\"page\"", pos);
            if (type_pos == std::string::npos) return "";
        }
        // ищем webSocketDebuggerUrl ПОСЛЕ этой позиции, но в пределах текущего объекта
        size_t ws_key = json.find("webSocketDebuggerUrl", type_pos);
        if (ws_key == std::string::npos) return "";
        size_t val_start = json.find("\"", json.find(":", ws_key)) + 1;
        size_t val_end = json.find("\"", val_start);
        return json.substr(val_start, val_end - val_start);
    }
}

std::string APP::page_parcer(std::string url) {
    const int PORT = 9222;
    std::string result;

    pid_t pid = fork();
    if (pid == -1) return "";

    if (pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        dup2(devnull, STDIN_FILENO);
        close(devnull);

        execlp("chromium", "chromium",
               "--headless", "--disable-gpu",
               "--no-sandbox", "--disable-setuid-sandbox",
               "--remote-debugging-port=9222",
               "--remote-debugging-address=127.0.0.1",
               "--disable-dev-shm-usage",
               url.c_str(), nullptr);
        _exit(127);
    }

    // ждём порт
    int sock = -1;
    for (int i = 0; i < 100; i++) {
        sock = connect_local(PORT);
        if (sock >= 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (sock < 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); return "ERR_NO_PORT"; }

    // GET /json
    std::string req = "GET /json HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    send(sock, req.c_str(), req.size(), 0);
    std::string json_list = read_all(sock, 5000);
    close(sock);

    std::string ws_url = find_page_ws_url(json_list);
    if (ws_url.empty()) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); return "ERR_NO_PAGE: " + json_list; }

    std::string path = ws_url.substr(ws_url.find("/devtools"));

    // подключаемся по WS
    sock = connect_local(PORT);
    if (sock < 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); return "ERR_CONNECT2"; }

    std::string handshake =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: 127.0.0.1:" + std::to_string(PORT) + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";
    send(sock, handshake.c_str(), handshake.size(), 0);

    // читаем хендшейк-ответ до \r\n\r\n (фиксированный таймаут, без read_all-до-разрыва)
    {
        timeval tv{2,0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        std::string hs;
        char c;
        while (hs.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = recv(sock, &c, 1, 0);
            if (n <= 0) break;
            hs += c;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::string js =
        "JSON.stringify(Array.from(document.querySelectorAll('*')).map(el=>{"
        "const r=el.getBoundingClientRect();"
        "return {tag:el.tagName,id:el.id,cls:el.className,"
        "x:Math.round(r.left),y:Math.round(r.top),"
        "w:Math.round(r.width),h:Math.round(r.height),"
        "text:(el.textContent||'').trim().substring(0,50)};"
        "}))";

    std::string escaped_js;
    for (char ch : js) {
        if (ch == '"' || ch == '\\') escaped_js += '\\';
        escaped_js += ch;
    }

    std::string cmd = "{\"id\":1,\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\""
                       + escaped_js + "\",\"returnByValue\":true}}";

    ws_send_text(sock, cmd);

    // читаем фреймы в цикле, пока не найдём "id":1 в payload (это ответ на нашу команду)
    std::string response;
    for (int i = 0; i < 20; i++) {
        std::string frame = ws_recv_frame(sock, 3000);
        if (frame.empty()) break;
        if (frame.find("\"id\":1") != std::string::npos) {
            response = frame;
            break;
        }
        // иначе это событие (например Page.frameStartedLoading) — пропускаем
    }

    if (response.empty()) {
        close(sock);
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        return "ERR_NO_RESPONSE";
    }

    // вытаскиваем result.result.value
    size_t pos = response.find("\"value\":\"");
    if (pos != std::string::npos) {
        pos += 9;
        std::string unescaped;
        for (size_t i = pos; i < response.size(); i++) {
            if (response[i] == '\\' && i+1 < response.size()) {
                char next = response[i+1];
                if (next == 'n') { unescaped += '\n'; i++; }
                else if (next == '"') { unescaped += '"'; i++; }
                else if (next == '\\') { unescaped += '\\'; i++; }
                else unescaped += response[i];
            } else if (response[i] == '"') {
                break;
            } else {
                unescaped += response[i];
            }
        }
        result = unescaped;
    } else {
        result = "ERR_NO_VALUE: " + response;
    }

    close(sock);

    kill(pid, SIGTERM);
    int status;
    for (int i = 0; i < 20; i++) {
        if (waitpid(pid, &status, WNOHANG) == pid) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);

    return result;
}

void APP::page_L() {
  std::string html = page_parcer("https://youtube.com");


  huinya = html; // это вывод на экран
};


void APP::page_draw() {
  CURSOR_TO(2, 1);
  std::cout << "\033[48;2;111;111;111m" << page << "\033[0m" << std::flush; page.clear();
};


        std::string APP::page_parcer(std::string url) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return "";
    
    pid_t pid = fork();
    if (pid == -1) { close(pipefd[0]); close(pipefd[1]); return ""; }
    
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        int devnull = open("/dev/null", O_RDONLY);
        dup2(devnull, STDIN_FILENO); close(devnull);
        
        execlp("chromium", "chromium", 
               "--headless", "--disable-gpu",
               "--disable-software-rasterizer",
               "--disable-dev-shm-usage",
               "--no-sandbox", "--disable-setuid-sandbox",
               "--virtual-time-budget=5000",
               "--run-all-compositor-stages-before-draw",
               "--dump-dom", url.c_str(), nullptr);
        _exit(127);
    }
    
    close(pipefd[1]);
    std::vector<char> buf(65536);
    std::string result;
    result.reserve(1048576);
    
    ssize_t n;
    while ((n = read(pipefd[0], buf.data(), buf.size())) > 0)
        result.append(buf.data(), n);
    close(pipefd[0]);
    
    // Таймаут 5 сек
    int status, waited = 0;
    while (waited < 5000) {
        if (waitpid(pid, &status, WNOHANG) == pid) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited += 100;
    }
    if (waited >= 5000) { kill(pid, SIGTERM); usleep(500000); kill(pid, SIGKILL); waitpid(pid, &status, 0); }
    
    return result;
}

void APP::page_L() {
  std::string html = page_parcer("https://lite.duckduckgo.com");


  huinya = html; // это вывод на экран
};


        std::string html = page_parcer("https://en.wikipedia.org/wiki/Main_Page");

      // // убираем <script>...</script>
    // html = std::regex_replace(html, std::regex(R"(<script[^>]*>[\s\S]*?</script>)"), " ");
    //
    // // убираем <style>...</style>
    // html = std::regex_replace(html, std::regex(R"(<style[^>]*>[\s\S]*?</style>)"), " ");
    //
    // // убираем svg иконки
    // html = std::regex_replace(html, std::regex(R"(<svg[\s\S]*?</svg>)"), " ");
    //
    // // убираем все остальные теги <...>
    // html = std::regex_replace(html, std::regex(R"(<[^>]*>)"), " ");
    //
    // // убираем HTML-сущности типа &nbsp; &amp; и т.п. (опционально, базовые)
    // html = std::regex_replace(html, std::regex(R"(&nbsp;)"), " ");
    // html = std::regex_replace(html, std::regex(R"(&amp;)"), "&");
    //
    // // убираем лишние пробелы/переводы строк
    // html = std::regex_replace(html, std::regex(R"(\s+)"), " ");

  // убераем все что в скобках
  html = std::regex_replace(html, std::regex(R"(<svg[\s\S]*?</svg>)"), "");



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
// ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
// // считаем ширину таба
// auto size_width_tab = [&]() -> int {
//   int available = (int)(percent(90, WIDTH_TERMINAL)); // 90% ширины на все табы
//   int tab_width = available / tab_count;              // делим поровну
//   int max_tab_width = (int)percent(15, WIDTH_TERMINAL); // максимум одного таба
//   return std::min(tab_width, max_tab_width);          // берём меньшее
// };
// // ложим туда символы
// std::string width_tab(size_width_tab(), '0');
//
//
// if (KEY('t')) {
//   tab_count++;
//   tab_focus = tab_count;
// }
// if (KEY('w') && tab_count > 1) {
//   tab_count--;
//   if (tab_focus >= tab_count) tab_focus = tab_count; // не выходим за границу
// }
// for (int i = 1; i < tab_count+1; i++) {
//   if (i == tab_focus) {
//     // Активная вкладка — белая
//     std::cout << "\033[4m\033[48;2;205;205;205m\033[30m" << width_tab << "\u2717\033[0m" << std::flush;
//   } else {
//     // Неактивная вкладка — чёрная
//     std::cout << "\033[4m\033[48;2;50;50;50m\033[38;2;255;255;255m" << width_tab << "\u2717\033[0m" << std::flush;
//   }
// }
//
// КУРСОР_НА(HEIGHT_TERMINAL, 1);
// std::cout << size_width_tab() << std::flush;
//
// monitor.clear();
// usleep(1000000 / 30);


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
  // 1. СКРЫВАЕМ КУРСОР ПЕРЕД ЦИКЛОМ
  std::cout << "\033[?25l" << std::flush;
  keyboard(true);  // включаем ввод с клавиатуры
  std::signal(SIGINT, signalHandler); // обработчик на нормальный выход
  int WIDTH_TERMINAL;
  int HEIGHT_TERMINAL;
  int monitor_size;
  std::string monitor;

  bool tab_active = false;
  while (1) {
  ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
  ПРОСЧИТЫВАЕМ_РАЗМЕР_ТЕРМИНАЛА

  // char input_buf[3] = {0};
  // int n = read(STDIN_FILENO, input_buf, sizeof(input_buf));
  // if (n == 2 && input_buf[0] == '\033' && input_buf[1] == 't') {
  //     tab_active = !tab_active; // переключаем состояние по Alt+T
  // }

  monitor_size = (HEIGHT_TERMINAL * WIDTH_TERMINAL) ;
  monitor.assign(monitor_size, ' ');

  std::cout << "\033[48;2;0;0;175m" << monitor << "\033[0m" << std::flush;

  ПЕРЕВОДИМ_КУРСОР_К_НАЧАЛУ_ТЕРМИНАЛА
  double percent_width_tab = percent(15, WIDTH_TERMINAL);
  std::string width_tab(percent_width_tab, '0');
  #define m_ "\033[4m"
  #define mx "\u2717"
  // std::cout << m_ << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << mx << "\033[0m" << std::flush;
  // std::cout << m_ << "\033[48;2;255;255;255m" << "\033[30m" << width_tab << mx << "\033[0m" << std::flush;
  // if (tab_active) {
  //   std::cout << m_ << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << mx << "\033[0m" << std::flush;
  // } else {
  //   std::cout << m_ << "\033[48;2;255;255;255m" << "\033[30m" << width_tab << mx << "\033[0m" << std::flush;
  // }
  //отладчик клавишь, дает показ что за код клавиши нажат
  char input_buf[8] = {0};
  int n = read(STDIN_FILENO, input_buf, sizeof(input_buf));
  if (n > 0) {
      for (int i = 0; i < n; ++i) {
          printf("[%d] ", (unsigned char)input_buf[i]);
      }
      printf("\n");
  }




  monitor.clear();
    usleep(16666);
  };
  keyboard(false);
};
// screen = screen + "\033[38;2;0;180;210;48;2;0;18;210m \033[0m";
// fflush(stdout);


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
  std::cout << "\033[4m" << "\033[48;2;0;0;0m" << "\033[38;2;255;255;255m" << width_tab << "\u2717" << "\033[0m" << std::flush;



  monitor.clear();
    usleep(16666);
  };
};
// screen = screen + "\033[38;2;0;180;210;48;2;0;18;210m \033[0m";
// fflush(stdout);
