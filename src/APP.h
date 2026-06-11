// APP.h
#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <iterator> // для size
#include <vector>
#include <iostream>
#include <utility> // для std::swap
#include <algorithm>
#include <utility>
#include <string>
#include <fstream>
#include <math.h>
#include <array>
#include <csignal> // Добавляем для работы с сигналами
#include <sys/ioctl.h>
#include <unistd.h>
#include <ostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <csignal>
#include <cstdlib>

#include "define.h"


class APP {
  public:
    APP();
    void keyboard(bool enable);
    // void signalHandler(int signum);
    void logica();
    void draw();
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
    std::string page = page_L();
    std::string monitor_L();
    std::string page_L();
    void monitor_draw();
    void tab_draw();
    void page_draw();

};

// APP::APP() {
//   tabs.push_back({"duck duck go", "lite.duckduckgo.com", true, false});
// };

extern APP* g_app_instance;
void signalHandler(int signum);
