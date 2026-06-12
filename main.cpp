#include "src/APP.h"

int main(){
    APP app;

    std::cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" << std::flush;
    while (1) {
      app.logica();
      app.draw();
      usleep(1000000 / 30);
    }

    return 0;
}

