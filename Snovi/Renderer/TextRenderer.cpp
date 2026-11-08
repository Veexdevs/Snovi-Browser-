#include "TextRenderer.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <set>
namespace snovi {
std::string safeTerminal(const std::string& s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        auto c=static_cast<unsigned char>(s[i]);
        if(c=='\n' || c=='\t' || (c>=32 && c<127)) {out+=static_cast<char>(c);continue;}
        // Keep well-formed UTF-8 while dropping raw control bytes and escape sequences.
        size_t n=c>=0xc2 && c<=0xdf ? 2 : c>=0xe0 && c<=0xef ? 3 : c>=0xf0 && c<=0xf4 ? 4 : 0;
        if(!n || i+n>s.size()) continue;
        bool valid=true; for(size_t j=1;j<n;++j) if((static_cast<unsigned char>(s[i+j])&0xc0)!=0x80) valid=false;
        auto second=static_cast<unsigned char>(s[i+1]);
        if((c==0xe0 && second<0xa0) || (c==0xed && second>=0xa0) || (c==0xf0 && second<0x90) || (c==0xf4 && second>=0x90)) valid=false;
        if(valid) {out.append(s,i,n);i+=n-1;}
    }
    return out;
}
Rendered renderText(const Document& document, int zoom) {
    Rendered result; std::string raw;
    const std::set<std::string> blocks={"p","div","section","article","header","footer","main","h1","h2","h3","h4","h5","h6","li","ul","ol","br","hr","tr","pre","blockquote"};
    std::function<void(const Node&)> walk=[&](const Node& n) {
        if(n.tag=="head" || n.tag=="script" || n.tag=="style" || n.tag=="template" || n.tag=="noscript") return;
        if(n.tag=="#text") {raw+=safeTerminal(n.text);return;}
        bool block=blocks.count(n.tag); if(block) raw+='\n';
        if(n.tag=="img") { auto a=n.attributes.find("alt"); if(a!=n.attributes.end()) raw+="[Imagem: "+safeTerminal(a->second)+"]"; }
        for(const auto& child:n.children) walk(*child);
        if(n.tag=="a") {auto a=n.attributes.find("href"); if(a!=n.attributes.end() && !a->second.empty()) {result.links.push_back(a->second); raw+=" ["+std::to_string(result.links.size())+"]";} }
        if(block) raw+='\n';
    }; walk(document.root);
    size_t width=static_cast<size_t>(std::clamp(80*100/std::clamp(zoom,50,200),40,160)); size_t col=0; bool pendingSpace=false, pendingNewline=false;
    for(unsigned char c:raw) {
        if(c=='\n') {pendingNewline=true;pendingSpace=false;continue;}
        if(c=='\r' || c=='\t' || c==' ') {pendingSpace=true;continue;}
        if(pendingNewline && !result.text.empty() && result.text.back()!='\n') {result.text+='\n';col=0;}
        else if(pendingSpace && col>0) {result.text+=' ';++col;}
        pendingSpace=false; pendingNewline=false;
        if(col>=width && (c&0xc0)!=0x80) {result.text+='\n';col=0;}
        result.text+=static_cast<char>(c); if((c&0xc0)!=0x80) ++col;
    }
    return result;
}
}
