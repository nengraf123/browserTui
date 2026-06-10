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







class APP {
  public:
    int browserInTerminal();
    void AI();
  private:
    #define p printf
    #define dbl double
};


