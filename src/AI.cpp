#include "APP.h"

// void APP::AI() {
// };

#include <vector>
#include <string>
#include <cstdint>
#include <unistd.h>
#include <cstdio>

// простой base64-энкодер
static std::string base64_encode(const std::vector<uint8_t>& data) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t n = (data[i] << 16) | (data[i+1] << 8) | data[i+2];
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];
        out += tbl[n & 63];
        i += 3;
    }
    size_t rem = data.size() - i;
    if (rem == 1) {
        uint32_t n = data[i] << 16;
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += "==";
    } else if (rem == 2) {
        uint32_t n = (data[i] << 16) | (data[i+1] << 8);
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];
        out += "=";
    }
    return out;
}

void APP::AI() {
    int w = 100, h = 100;
    // заливаем буфер одним цветом, например красным (R,G,B)
    std::vector<uint8_t> pixels(w * h * 3);
    for (int i = 0; i < w * h; ++i) {
        pixels[i*3 + 0] = 255; // R
        pixels[i*3 + 1] = 0;   // G
        pixels[i*3 + 2] = 0;   // B
    }

    std::string b64 = base64_encode(pixels);

    // отправляем чанками по 4096 байт base64-данных
    const size_t chunk_size = 4096;
    size_t pos = 0;
    bool first = true;

    while (pos < b64.size()) {
        size_t len = std::min(chunk_size, b64.size() - pos);
        std::string chunk = b64.substr(pos, len);
        pos += len;
        bool more = (pos < b64.size());

std::string esc = "\033_G";
if (first) {
    esc += "a=T,f=24,s=" + std::to_string(w) + ",v=" + std::to_string(h) + ",";
    first = false;
}
esc += "m=" + std::string(more ? "1" : "0");
esc += ";" + chunk;
esc += "\033\\";

        write(STDOUT_FILENO, esc.c_str(), esc.size());
    }
    fflush(stdout);
    while (1) {
    }
}
