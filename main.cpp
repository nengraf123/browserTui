#include "src/APP.h"

int main(){
    APP app;
    
    while (true) {
      app.logica();
      app.draw();
      usleep(1000000 / 30);
    }

    return 0;
}


