#include "AdBlock.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
namespace snovi {
static bool matches(const std::set<std::string>& hosts, const std::string& host) {
    if(hosts.count(host)) return true;
    for(size_t p=host.find('.');p!=std::string::npos;p=host.find('.',p+1)) if(hosts.count(host.substr(p+1))) return true;
    return false;
}
bool AdBlock::allow(const Url& url) {
    if (!enabled || matches(allowed_hosts,url.host)) return true;
    if (matches(blocked_hosts,url.host)) { ++blocked; return false; }
    return true;
}
void AdBlock::loadHosts(const std::string& path) {
    std::ifstream in(path); if(!in) throw std::runtime_error("Não foi possível abrir a lista: "+path);
    std::set<std::string> parsed; std::string line;
    while(std::getline(in,line)) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        if(line.empty() || line[0]=='#') continue;
        std::transform(line.begin(),line.end(),line.begin(),[](unsigned char c){return std::tolower(c);});
        if(line.size()>253 || line.front()=='.' || line.back()=='.' || line.find("..")!=std::string::npos || !std::all_of(line.begin(),line.end(),[](unsigned char c){return std::isalnum(c)||c=='.'||c=='-';})) throw std::runtime_error("Domínio inválido na lista: "+line);
        parsed.insert(line);
    }
    blocked_hosts=std::move(parsed);
}
}
