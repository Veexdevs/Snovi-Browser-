#pragma once
#include "HTML/Document.h"
#include <string>
#include <vector>
namespace snovi {
struct Rendered { std::string text; std::vector<std::string> links; };
Rendered renderText(const Document& document, int zoom=100);
std::string safeTerminal(const std::string& text);
}
