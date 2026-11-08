#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace snovi {
struct Node {
    std::string tag, text;
    std::map<std::string,std::string> attributes;
    std::vector<std::unique_ptr<Node>> children;
};
struct Document { Node root; };
Document parseHtml(const std::string& html);
std::string decodeEntities(const std::string& text);
}
