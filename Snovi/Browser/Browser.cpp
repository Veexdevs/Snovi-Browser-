#include "Browser.h"
#include <algorithm>
#include <stdexcept>
namespace snovi {
Page Browser::fetch(const Url& url,int zoom) {
    HttpClient client(filter); auto response=client.get(url);
    auto content=response.headers.find("content-type");
    if(content!=response.headers.end() && content->second.find("text/html")==std::string::npos && content->second.find("application/xhtml+xml")==std::string::npos && content->second.find("text/plain")==std::string::npos) throw std::runtime_error("Tipo de conteúdo ainda não suportado: "+content->second);
    Rendered rendered;
    if(content!=response.headers.end() && content->second.find("text/plain")!=std::string::npos) rendered.text=safeTerminal(response.body);
    else rendered=renderText(parseHtml(response.body),zoom);
    return {std::move(response),std::move(rendered)};
}
void Browser::navigate(const std::string& url) {
    auto& tab=tabs.at(currentTab); auto page=fetch(Url::parse(url),tab.zoom);
    if(!tab.history.empty()) tab.history.erase(tab.history.begin()+tab.current+1,tab.history.end());
    tab.history.push_back(std::move(page)); tab.current=tab.history.size()-1;
}
Page* Browser::page() {auto& tab=tabs.at(currentTab); return tab.history.empty()?nullptr:&tab.history.at(tab.current);}
void Browser::reload() {auto& tab=tabs.at(currentTab); if(!page()) throw std::runtime_error("Nenhuma página aberta"); auto replacement=fetch(page()->response.url,tab.zoom); tab.history[tab.current]=std::move(replacement);}
void Browser::back() {auto& tab=tabs.at(currentTab); if(tab.history.empty() || !tab.current) throw std::runtime_error("Sem página anterior"); --tab.current;}
void Browser::forward() {auto& tab=tabs.at(currentTab); if(tab.history.empty() || tab.current+1>=tab.history.size()) throw std::runtime_error("Sem próxima página"); ++tab.current;}
void Browser::setZoom(int z) {auto& tab=tabs.at(currentTab); tab.zoom=std::clamp(z,50,200); if(auto* p=page()) {auto type=p->response.headers.find("content-type"); if(type==p->response.headers.end() || type->second.find("text/plain")==std::string::npos) p->rendered=renderText(parseHtml(p->response.body),tab.zoom);} }
}
