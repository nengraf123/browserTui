// APP.h
#pragma once 

#include "include.h"
#include "define.h"



class APP {
  public:
    APP();
    void keyboard(bool enable);
    // void signalHandler(int signum);
    void logica();
    void draw();
    std::string huinya;
  private:
    int WIDTH_TERMINAL=terminal_size_WIDTH();
    int HEIGHT_TERMINAL=terminal_size_HEIGHT();
    int terminal_size_WIDTH();
    int terminal_size_HEIGHT();
    struct Tab {
      std::string title = "Новая вкладка"; // Значение по умолчанию
      std::string url = "";
      bool is_active;
      bool is_loading;
    };
    std::vector<Tab> tabs;
    std::string monitor = monitor_L();
    std::string page = page_size_L();
    std::string monitor_L();
    std::string page_size_L();
    void monitor_draw();
    void tab_draw();
    void page_draw();
    void page_L();
    std::string page_parcer(std::string url);
    std::string html_parcer(std::string url);
    std::string dom = html_parcer("https://lite.duckduckgo.com");


};

// APP::APP() {
//   tabs.push_back({"duck duck go", "lite.duckduckgo.com", true, false});
// };

extern APP* g_app_instance;
void signalHandler(int signum);
