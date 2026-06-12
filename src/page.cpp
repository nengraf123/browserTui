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

  // 1. Универсальная лямбда: принимает строку HTML и ТЕГ, который ищем
  auto find_and_react = [&](const std::string& text, std::string_view tag) {
    // Собираем открывающий тег (например, "<center")
    // Ищем без закрывающей ">", чтобы находить теги с атрибутами, например <div class="bg">
    std::string target = "<" + std::string(tag);
    size_t pos = text.find(target);

    while (pos != std::string::npos) {
      
      // 2. Описываем реакцию под каждый конкретный тег
      if (tag == "center") {
        page.replace(page.size() / 2, 5, "HELLO");
      } 
      else if (tag == "a") {

      } 
      else if (tag == "div") {
          // Твоя логика для блоков
      }

      // Ищем следующее вхождение этого же тега
      pos = text.find(target, pos + target.length());
    }
  };

  // 3. ПРОСТО ВЫЗЫВАЕШЬ ДЛЯ ВСЕХ НУЖНЫХ ТЕГОВ ПО ОЧЕРЕДИ:
  find_and_react(dom, "center");
  find_and_react(dom, "a");
  find_and_react(dom, "div");
  find_and_react(dom, "img");
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
