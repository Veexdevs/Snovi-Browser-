#include "Browser/Browser.h"
#include <algorithm>
#include <csignal>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace snovi;
namespace {
void help() {
    std::cout << "\nSNOVI • Browse beyond.\n"
              << "abrir URL | link N | voltar | avancar | atualizar | fonte\n"
              << "aba nova | aba N | abas | zoom +|-|100 | favorito | favoritos\n"
              << "historico | adblock on|off|status | lista ARQUIVO | permitir DOMINIO\n"
              << "endereco | inicio | ajuda | sair\n";
}
void show(Browser& b) {
    auto* p=b.page();
    std::cout << "\n────────────────────────────────────────────\n";
    if(!p) {std::cout << "SNOVI  ·  Browse beyond.\nDigite 'abrir https://exemplo.com' ou 'ajuda'.\n";return;}
    std::cout << safeTerminal(p->response.url.str()) << "  ·  HTTP " << p->response.status << "  ·  zoom " << b.tabs[b.currentTab].zoom << "%\n\n" << p->rendered.text << "\n";
}
std::string arg(const std::string& s) {auto p=s.find(' '); return p==std::string::npos ? "" : s.substr(p+1);}
size_t number(const std::string& s) {size_t used=0; auto n=std::stoull(s,&used); if(used!=s.size() || n==0) throw std::runtime_error("Número inválido"); return n;}
}
int main(int argc,char** argv) {
    std::signal(SIGPIPE,SIG_IGN);
    Browser b; help();
    if(argc>1) try {std::cout << "Carregando...\n"; b.navigate(argv[1]);} catch(const std::exception& e) {std::cerr << "Erro: " << e.what() << "\n";}
    show(b); std::string line;
    while(std::cout << "\nsnovi > " && std::getline(std::cin,line)) {
        try {
            auto command=line.substr(0,line.find(' ')); auto value=arg(line); bool display=false;
            if(command=="sair") break;
            else if(command=="ajuda" || command=="menu") help();
            else if(command=="abrir") {std::cout << "Carregando...\n"; b.navigate(value);display=true;}
            else if(command=="link") {auto* p=b.page(); auto n=number(value); if(!p || n>p->rendered.links.size()) throw std::runtime_error("Link inexistente"); auto u=p->response.url.resolve(p->rendered.links[n-1]); std::cout << "Carregando...\n"; b.navigate(u.str());display=true;}
            else if(command=="voltar") {b.back();display=true;}
            else if(command=="avancar") {b.forward();display=true;}
            else if(command=="atualizar") {std::cout << "Carregando...\n"; b.reload();display=true;}
            else if(command=="inicio") {b.tabs[b.currentTab]=Tab{};display=true;}
            else if(command=="fonte") {auto* p=b.page(); if(!p) throw std::runtime_error("Nenhuma página aberta"); std::cout << "Fonte recebida de " << safeTerminal(p->response.url.str()) << ":\n" << safeTerminal(p->response.body) << "\n";}
            else if(command=="endereco") {auto* p=b.page(); if(!p) throw std::runtime_error("Nenhuma página aberta"); std::cout << safeTerminal(p->response.url.str()) << "\n";}
            else if(command=="zoom") {auto z=b.tabs[b.currentTab].zoom; if(value=="+") z+=10; else if(value=="-") z-=10; else if(value=="100") z=100; else throw std::runtime_error("Use zoom +, zoom - ou zoom 100"); b.setZoom(z);display=true;}
            else if(command=="aba") {if(value=="nova") {b.tabs.push_back(Tab{});b.currentTab=b.tabs.size()-1;} else {auto n=number(value); if(n>b.tabs.size()) throw std::runtime_error("Aba inexistente");b.currentTab=n-1;} display=true;}
            else if(command=="abas") {for(size_t i=0;i<b.tabs.size();++i) {auto& t=b.tabs[i];std::cout << (i==b.currentTab?"* ":"  ") << i+1 << "  " << (t.history.empty()?"Início":safeTerminal(t.history[t.current].response.url.str())) << "\n";}}
            else if(command=="favorito") {auto* p=b.page(); if(!p) throw std::runtime_error("Nenhuma página aberta"); auto u=p->response.url.str(); if(std::find(b.favorites.begin(),b.favorites.end(),u)==b.favorites.end()) b.favorites.push_back(u); std::cout << "Favorito adicionado nesta sessão.\n";}
            else if(command=="favoritos") {for(auto& u:b.favorites) std::cout << safeTerminal(u) << "\n";}
            else if(command=="historico") {auto& t=b.tabs[b.currentTab]; for(auto& p:t.history) std::cout << safeTerminal(p.response.url.str()) << "\n";}
            else if(command=="adblock") {if(value=="on") b.filter.enabled=true; else if(value=="off") b.filter.enabled=false; else if(value!="status") throw std::runtime_error("Use adblock on, off ou status"); std::cout << "AdBlock " << (b.filter.enabled?"ativo":"inativo") << " · " << b.filter.blocked << " bloqueadas · " << b.filter.blocked_hosts.size() << " domínios na lista\n";}
            else if(command=="lista") {b.filter.loadHosts(value);std::cout << "Lista carregada: " << b.filter.blocked_hosts.size() << " domínios.\n";}
            else if(command=="permitir") {auto host=Url::parse("http://"+value+"/").host;b.filter.allowed_hosts.insert(host);std::cout << "Exceção adicionada nesta sessão.\n";}
            else if(!command.empty()) std::cout << "Comando desconhecido. Digite ajuda.\n";
            if(display) show(b);
        } catch(const std::exception& e) {std::cerr << "Erro: " << safeTerminal(e.what()) << "\n";}
    }
}
