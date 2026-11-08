#pragma once
#include <cstddef>
#include <set>
#include <string>
#include "Network/Url.h"
namespace snovi {
class AdBlock {
public:
    bool enabled = true;
    size_t blocked = 0;
    std::set<std::string> blocked_hosts, allowed_hosts;
    bool allow(const Url& url);
    void loadHosts(const std::string& path);
};
}
