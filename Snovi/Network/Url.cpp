#include "Url.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace snovi {
static bool bad(char c) { return static_cast<unsigned char>(c) <= 32 || c == 127; }
Url Url::parse(const std::string& input) {
    if (input.size() > 8192 || std::any_of(input.begin(), input.end(), bad)) throw std::runtime_error("URL inválida");
    auto p = input.find("://");
    if (p == std::string::npos) throw std::runtime_error("A URL deve começar com http:// ou https://");
    Url u; u.scheme = input.substr(0,p);
    std::transform(u.scheme.begin(),u.scheme.end(),u.scheme.begin(),[](unsigned char c){return std::tolower(c);});
    if (u.scheme != "http" && u.scheme != "https") throw std::runtime_error("Esquema não suportado");
    auto end = input.find_first_of("/?#",p+3);
    auto auth = input.substr(p+3, end == std::string::npos ? std::string::npos : end-p-3);
    if (auth.empty() || auth.find('@') != std::string::npos || auth.find('\\') != std::string::npos) throw std::runtime_error("Autoridade inválida");
    if (auth.front() == '[') {
        auto close = auth.find(']');
        if (close == std::string::npos) throw std::runtime_error("IPv6 inválido");
        u.host = auth.substr(1,close-1);
        if (close+1 < auth.size()) { if(auth[close+1]!=':') throw std::runtime_error("Porta inválida"); u.port=auth.substr(close+2); }
    } else {
        auto colon = auth.find(':');
        if (colon != std::string::npos && auth.find(':',colon+1) != std::string::npos) throw std::runtime_error("IPv6 deve usar colchetes");
        u.host=auth.substr(0,colon);
        if(colon!=std::string::npos) u.port=auth.substr(colon+1);
    }
    if (u.host.empty() || (auth.back()==':')) throw std::runtime_error("Host ou porta inválidos");
    std::transform(u.host.begin(),u.host.end(),u.host.begin(),[](unsigned char c){return std::tolower(c);});
    if (u.port.empty()) u.port = u.scheme == "https" ? "443" : "80";
    if (!std::all_of(u.port.begin(),u.port.end(),[](unsigned char c){return std::isdigit(c);}) || u.port.size()>5 || std::stoi(u.port)==0 || std::stoi(u.port)>65535) throw std::runtime_error("Porta inválida");
    u.target = end == std::string::npos ? "/" : input.substr(end);
    auto hash=u.target.find('#'); if(hash!=std::string::npos) u.target.erase(hash);
    if(u.target.empty()) u.target="/"; else if(u.target[0]=='?') u.target="/"+u.target;
    return u;
}
std::string Url::authority() const { return (host.find(':')!=std::string::npos ? "["+host+"]" : host) + ((scheme=="http" && port=="80") || (scheme=="https" && port=="443") ? "" : ":"+port); }
std::string Url::str() const { return scheme+"://"+authority()+target; }
Url Url::resolve(const std::string& ref) const {
    if (ref.find("://") != std::string::npos) return parse(ref);
    if (ref.rfind("//",0)==0) return parse(scheme+":"+ref);
    if (ref.find(':') != std::string::npos && ref.find(':') < ref.find_first_of("/?#")) throw std::runtime_error("Esquema não suportado");
    if (ref.empty() || ref[0]=='#') return *this;
    std::string base=scheme+"://"+authority();
    if(ref[0]=='/') return parse(base+ref);
    auto path=target.substr(0,target.find('?'));
    if(ref[0]=='?') return parse(base+path+ref);
    auto slash=path.rfind('/'); path=path.substr(0,slash+1)+ref;
    // Normalize dot segments without interpreting percent-encoded bytes.
    auto q=path.find_first_of("?#"); auto suffix=q==std::string::npos ? "" : path.substr(q); path=path.substr(0,q);
    std::vector<std::string> parts; size_t start=1;
    while(start<=path.size()) { auto stop=path.find('/',start); auto part=path.substr(start,stop-start); if(part=="..") { if(!parts.empty()) parts.pop_back(); } else if(!part.empty() && part!=".") parts.push_back(part); if(stop==std::string::npos) break; start=stop+1; }
    path="/"; for(size_t i=0;i<parts.size();++i) { if(i) path+="/"; path+=parts[i]; } if(!ref.empty() && ref.back()=='/' && path.back()!='/') path+='/';
    return parse(base+path+suffix);
}
}
