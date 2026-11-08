#pragma once
#include <string>

namespace snovi {
struct Url {
    std::string scheme, host, port, target;
    static Url parse(const std::string& input);
    std::string str() const;
    std::string authority() const;
    Url resolve(const std::string& reference) const;
};
}
