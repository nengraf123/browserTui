#include "APP.h"

std::string APP::page_parcer(std::string url) {
    std::string cmd = "chromium --headless --disable-gpu --dump-dom \"" + url + "\" 2>/dev/null";
    
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    return result; // это уже готовый HTML после выполнения JS
}


void APP::page_L() {
  std::string html = page_parcer("https://youtube.com");
  huinya = html;
};

void APP::page_draw() {
  CURSOR_TO(2, 1);
  std::cout << "\033[48;2;111;111;111m" << page << "\033[0m" << std::flush; page.clear();
};

