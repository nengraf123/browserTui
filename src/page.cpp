#include "APP.h"

#include <gumbo.h>
#include <curl/curl.h>
#include <string>

std::string APP::html_parcer(std::string url) {
    // --- скачивание HTML ---
    std::string html;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](void* ptr, size_t size, size_t nmemb, std::string* data) -> size_t {
                data->append((char*)ptr, size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    // --- парсинг и вывод структуры видимых элементов ---
    GumboOutput* output = gumbo_parse(html.c_str());
    std::string result;

    std::function<void(GumboNode*, int)> walk = [&](GumboNode* node, int depth) {
        if (node->type == GUMBO_NODE_TEXT) {
            std::string text = node->v.text.text;
            // пропускаем чисто пробельный текст
            bool only_space = true;
            for (char c : text) if (!isspace((unsigned char)c)) { only_space = false; break; }
            if (only_space) return;

            for (int j = 0; j < depth; j++) result += "  ";
            result += "TEXT: " + text + "\n";
            return;
        }
        if (node->type != GUMBO_NODE_ELEMENT) return;

        GumboTag tag = node->v.element.tag;
        if (tag == GUMBO_TAG_SCRIPT || tag == GUMBO_TAG_STYLE ||
            tag == GUMBO_TAG_HEAD || tag == GUMBO_TAG_NOSCRIPT) return;

        const char* tagname = gumbo_normalized_tagname(tag);
        for (int j = 0; j < depth; j++) result += "  ";
        result += "<" + std::string(tagname);

        // src / href атрибуты
        GumboVector* attrs = &node->v.element.attributes;
        for (unsigned k = 0; k < attrs->length; k++) {
            GumboAttribute* attr = (GumboAttribute*)attrs->data[k];
            std::string name = attr->name;
            if (name == "src" || name == "href") {
                result += " " + name + "=\"" + attr->value + "\"";
            }
        }
        result += ">\n";

        GumboVector* children = &node->v.element.children;
        for (unsigned i = 0; i < children->length; i++) {
            walk((GumboNode*)children->data[i], depth + 1);
        }
    };

    walk(output->root, 0);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return result;
}



// std::string APP::page_parcer(std::string url) {
//   return 0;
// };

void APP::page_L() {
  huinya = dom;
  // size_t render_pos = 0;
  // std::istringstream stream(dom);
  // std::string line;
  // std::vector<int> center_stack;
  //
  // while (std::getline(stream, line)) {
  //   size_t first = line.find_first_not_of(" ");
  //   if (first == std::string::npos) continue;
  //   int depth = first / 2;
  //   std::string token = line.substr(first);
  //
  //   while (!center_stack.empty() && depth <= center_stack.back())
  //     center_stack.pop_back();
  //   bool centering = !center_stack.empty();
  //
  //   // вспомогательная лямбда: положить текст с учётом центрирования
  //   auto put_text = [&](const std::string& text) {
  //     size_t pos = render_pos;
  //     if (centering) {
  //       size_t line_start = (render_pos / WIDTH_TERMINAL) * WIDTH_TERMINAL;
  //       size_t offset = (text.size() < (size_t)WIDTH_TERMINAL)
  //                       ? (WIDTH_TERMINAL - text.size()) / 2 : 0;
  //       pos = line_start + offset;
  //     }
  //     if (pos + text.size() <= page.size())
  //       page.replace(pos, text.size(), text);
  //     render_pos = pos + text.size();
  //   };
  //
  //   if (token.rfind("<center", 0) == 0) {
  //     center_stack.push_back(depth);
  //   }
  //   else if (token.rfind("<br", 0) == 0) {
  //     render_pos = ((render_pos / WIDTH_TERMINAL) + 1) * WIDTH_TERMINAL;
  //   }
  //   else if (token.rfind("TEXT:", 0) == 0) {
  //     put_text(token.substr(6));
  //   }
  //   else if (token.rfind("<a", 0) == 0) {
  //     // ссылка — текст внутри будет дальше как TEXT:, просто помечаем стиль
  //     // (можно дописать ANSI-подсветку перед текстом, если нужно)
  //   }
  //   else if (token.rfind("<img", 0) == 0) {
  //     put_text("[IMG]");
  //   }
  //   else if (token.rfind("<input", 0) == 0) {
  //     put_text("[______]");
  //   }
  //   else if (token.rfind("<span", 0) == 0 ||
  //            token.rfind("<form", 0) == 0 ||
  //            token.rfind("<p", 0) == 0) {
  //     // контейнеры — сами по себе ничего не рисуют,
  //     // их содержимое отрисуется через TEXT:/вложенные теги
  //   }
  // }
}


void APP::page_draw() {
  CURSOR_TO(1, 1);
  std::cout << "\033[48;2;111;111;111m" << page << "\033[0m" << std::flush; page.clear();
};

// <html>
//   <body>
//     <a>
//     <center>
//       <br>
//       <span>
//         TEXT: DuckDuckGo
//       <br>
//       <br>
//       <form>
//         <input>
//         <input>
//       <br>
//       <p>
