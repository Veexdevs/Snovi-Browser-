#include "Network/Url.h"
#include "HTML/Document.h"
#include "Renderer/TextRenderer.h"
#include "AdBlock/AdBlock.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
using namespace snovi;
int main() {
    auto u=Url::parse("https://Example.com:443/a/b?x=1#fragment");
    assert(u.str()=="https://example.com/a/b?x=1");
    assert(u.resolve("../c?q=1").str()=="https://example.com/c?q=1");
    assert(u.resolve("?a=2").str()=="https://example.com/a/b?a=2");
    assert(Url::parse("http://[::1]:8080/").authority()=="[::1]:8080");
    bool threw=false;try {Url::parse("http://example.com\r\nInjected:1/");} catch(...) {threw=true;} assert(threw);
    threw=false;try {u.resolve("javascript:alert(1)");} catch(...) {threw=true;} assert(threw);
    auto doc=parseHtml("<!doctype html><html><head><title>hidden</title></head><body><h1>Olá &amp; mundo</h1><script>bad()</script><p>Veja <a href='../x?a=1&amp;b=2'>o link</a><img alt='foto'></p></body></html>");
    auto r=renderText(doc); assert(r.text.find("Olá & mundo")!=std::string::npos); assert(r.text.find("bad()")==std::string::npos);assert(r.text.find("hidden")==std::string::npos); assert(r.links.size()==1 && r.links[0]=="../x?a=1&b=2"); assert(r.text.find("Imagem: foto")!=std::string::npos);
    assert(safeTerminal("a\x1b[31mb")=="a[31mb");
    AdBlock f; f.blocked_hosts.insert("ads.example.com"); assert(!f.allow(Url::parse("https://sub.ads.example.com/"))); assert(f.blocked==1); assert(f.allow(Url::parse("https://notads.example.com/"))); f.allowed_hosts.insert("sub.ads.example.com"); assert(f.allow(Url::parse("https://sub.ads.example.com/"))); f.enabled=false;assert(f.allow(Url::parse("https://ads.example.com/")));
    std::cout << "Core tests passed\n";
}
