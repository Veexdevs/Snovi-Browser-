#pragma once
#include "Url.h"
#include "AdBlock/AdBlock.h"
#include <map>
namespace snovi {
struct Response { Url url; int status; std::map<std::string,std::string> headers; std::string body; };
class HttpClient {
public:
    explicit HttpClient(AdBlock& filter): filter_(filter) {}
    Response get(const Url& url);
private:
    AdBlock& filter_;
    Response request(const Url& url);
};
}
