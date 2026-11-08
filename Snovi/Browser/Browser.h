#pragma once
#include "Network/Http.h"
#include "Renderer/TextRenderer.h"
#include <vector>
namespace snovi {
struct Page { Response response; Rendered rendered; };
struct Tab { std::vector<Page> history; size_t current=0; int zoom=100; };
class Browser {
public:
    AdBlock filter;
    std::vector<Tab> tabs{Tab{}};
    size_t currentTab=0;
    std::vector<std::string> favorites;
    void navigate(const std::string& url);
    void reload();
    void back();
    void forward();
    void setZoom(int zoom);
    Page* page();
private:
    Page fetch(const Url& url, int zoom);
};
}
