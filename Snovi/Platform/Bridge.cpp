// Small C ABI: the UI talks to the actual C++ engine, not a browser widget.
#include "Browser/Browser.h"
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

namespace {
using namespace snovi;
std::string quote(const std::string& value) {
    static const char hex[]="0123456789abcdef";
    std::string out="\"";
    for (unsigned char c : value) {
        if (c=='"' || c=='\\') { out+='\\'; out+=static_cast<char>(c); }
        else if (c<32) { out+="\\u00"; out+=hex[c>>4]; out+=hex[c&15]; }
        else out+=static_cast<char>(c);
    }
    return out+'"';
}
template<class T> void strings(std::ostringstream& o,const T& values) {
    o << '['; bool first=true; for(const auto& value:values) {if(!first) o<<','; first=false; o<<quote(value);} o<<']';
}
size_t indexOf(const std::string& input) {
    size_t pos=0; auto result=std::stoull(input,&pos);
    if(pos!=input.size()) throw std::runtime_error("Índice inválido");
    return static_cast<size_t>(result);
}
std::string state(Browser& b,const std::string& error) {
    std::ostringstream o; auto* page=b.page(); auto& tab=b.tabs.at(b.currentTab);
    o << "{\"error\":" << quote(error) << ",\"currentTab\":" << b.currentTab << ",\"tabs\":[";
    for(size_t i=0;i<b.tabs.size();++i) {
        if(i) o<<','; const auto& t=b.tabs[i];
        o << (t.history.empty()?"null":quote(t.history[t.current].response.url.str()));
    }
    o << "],\"url\":" << quote(page?page->response.url.str():"")
      << ",\"status\":" << (page?page->response.status:0) << ",\"zoom\":" << tab.zoom
      << ",\"canBack\":" << (!tab.history.empty() && tab.current>0?"true":"false")
      << ",\"canForward\":" << (!tab.history.empty() && tab.current+1<tab.history.size()?"true":"false")
      << ",\"text\":" << quote(page?page->rendered.text:"")
      << ",\"source\":" << quote(page?page->response.body:"") << ",\"links\":";
    if(page) strings(o,page->rendered.links); else o<<"[]";
    o << ",\"history\":["; for(size_t i=0;i<tab.history.size();++i) {if(i) o<<','; o<<quote(tab.history[i].response.url.str());} o<<"],\"favorites\":"; strings(o,b.favorites);
    o << ",\"adblockEnabled\":" << (b.filter.enabled?"true":"false") << ",\"blocked\":" << b.filter.blocked << ",\"filterCount\":" << b.filter.blocked_hosts.size() << '}';
    return o.str();
}
}
extern "C" {
void* snovi_create() { try {
#ifndef _WIN32
    std::signal(SIGPIPE,SIG_IGN);
#endif
 return new snovi::Browser();} catch(...) {return nullptr;} }
void snovi_destroy(void* handle) {delete static_cast<snovi::Browser*>(handle);}
void snovi_free(char* value) {std::free(value);}
char* snovi_execute(void* handle,const char* action,const char* argument) {
    if(!handle || !action) return nullptr;
    auto& b=*static_cast<snovi::Browser*>(handle); std::string a(action), v=argument?argument:"", error;
    try {
        if(a=="state") {}
        else if(a=="navigate") b.navigate(v);
        else if(a=="link") {auto* p=b.page(); auto n=indexOf(v); if(!p || n>=p->rendered.links.size()) throw std::runtime_error("Link inexistente"); auto url=p->response.url.resolve(p->rendered.links[n]); b.navigate(url.str());}
        else if(a=="reload") b.reload();
        else if(a=="back") b.back();
        else if(a=="forward") b.forward();
        else if(a=="home") b.tabs[b.currentTab]=snovi::Tab{};
        else if(a=="new_tab") {b.tabs.emplace_back(); b.currentTab=b.tabs.size()-1;}
        else if(a=="select_tab") {auto i=indexOf(v); if(i>=b.tabs.size()) throw std::runtime_error("Aba inexistente"); b.currentTab=i;}
        else if(a=="close_tab") {auto i=indexOf(v); if(i>=b.tabs.size()) throw std::runtime_error("Aba inexistente"); b.tabs.erase(b.tabs.begin()+i); if(b.tabs.empty()) b.tabs.emplace_back(); if(i<b.currentTab) --b.currentTab; else if(b.currentTab>=b.tabs.size()) b.currentTab=b.tabs.size()-1;}
        else if(a=="zoom") {auto z=std::stoi(v); b.setZoom(z);}
        else if(a=="favorite") {auto* p=b.page(); if(!p) throw std::runtime_error("Nenhuma página aberta"); auto u=p->response.url.str(); auto it=std::find(b.favorites.begin(),b.favorites.end(),u); if(it==b.favorites.end()) b.favorites.push_back(u); else b.favorites.erase(it);}
        else if(a=="adblock") b.filter.enabled=!b.filter.enabled;
        else if(a=="filter") b.filter.loadHosts(v);
        else if(a=="allow") { auto host=snovi::Url::parse("http://"+v+"/").host; b.filter.allowed_hosts.insert(host); }
        else throw std::runtime_error("Operação desconhecida");
    } catch(const std::exception& e) {error=e.what();}
    try {auto result=state(b,error); auto* out=static_cast<char*>(std::malloc(result.size()+1)); if(out) std::memcpy(out,result.c_str(),result.size()+1); return out;} catch(...) {return nullptr;}
}
}
