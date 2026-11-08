#include "Document.h"
#include <algorithm>
#include <cctype>
#include <set>
namespace snovi {
namespace {
std::string lower(std::string s) { std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return std::tolower(c);}); return s; }
bool space(char c) {return std::isspace(static_cast<unsigned char>(c));}
bool nameChar(char c) {return std::isalnum(static_cast<unsigned char>(c)) || c=='-' || c=='_' || c==':';}
void utf8(std::string& out, unsigned int c) {
    if(c==0 || c>0x10ffff || (c>=0xd800 && c<=0xdfff)) c=0xfffd;
    if(c<0x80) out+=static_cast<char>(c);
    else if(c<0x800) {out+=static_cast<char>(0xc0|(c>>6));out+=static_cast<char>(0x80|(c&63));}
    else if(c<0x10000) {out+=static_cast<char>(0xe0|(c>>12));out+=static_cast<char>(0x80|((c>>6)&63));out+=static_cast<char>(0x80|(c&63));}
    else {out+=static_cast<char>(0xf0|(c>>18));out+=static_cast<char>(0x80|((c>>12)&63));out+=static_cast<char>(0x80|((c>>6)&63));out+=static_cast<char>(0x80|(c&63));}
}
}
std::string decodeEntities(const std::string& s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]!='&') {out+=s[i]; continue;}
        auto end=s.find(';',i+1); if(end==std::string::npos || end-i>12) {out+='&';continue;}
        auto e=s.substr(i+1,end-i-1); bool found=true;
        if(e=="amp") out+='&'; else if(e=="lt") out+='<'; else if(e=="gt") out+='>'; else if(e=="quot") out+='"'; else if(e=="apos" || e=="#39") out+='\''; else if(e=="nbsp") out+=' ';
        else if(e.size()>1 && e[0]=='#') { try {size_t used=0; bool hex=e.size()>2 && (e[1]=='x'||e[1]=='X'); auto digits=e.substr(hex?2:1); auto n=std::stoul(digits,&used,hex?16:10); if(used!=digits.size() || n>0x10ffff) found=false; else utf8(out,static_cast<unsigned int>(n));} catch(...) {found=false;} }
        else found=false;
        if(found) i=end; else out+='&';
    } return out;
}
Document parseHtml(const std::string& html) {
    Document doc; doc.root.tag="#document"; std::vector<Node*> stack{&doc.root};
    const std::set<std::string> voids={"area","base","br","col","embed","hr","img","input","link","meta","param","source","track","wbr"};
    size_t p=0, nodes=0;
    auto text=[&](const std::string& s) { if(s.empty() || nodes>=100000) return; auto n=std::make_unique<Node>(); n->tag="#text"; n->text=decodeEntities(s); stack.back()->children.push_back(std::move(n)); ++nodes; };
    while(p<html.size() && nodes<100000) {
        if(html[p]!='<') { auto end=html.find('<',p); text(html.substr(p,end-p)); if(end==std::string::npos) break; p=end; continue; }
        if(html.compare(p,4,"<!--")==0) {auto end=html.find("-->",p+4); if(end==std::string::npos) break; p=end+3; continue;}
        if(p+1<html.size() && (html[p+1]=='!' || html[p+1]=='?')) {auto end=html.find('>',p+2); if(end==std::string::npos) break; p=end+1; continue;}
        size_t i=p+1; bool closing=i<html.size() && html[i]=='/'; if(closing) ++i; size_t begin=i; while(i<html.size() && nameChar(html[i])) ++i;
        if(i==begin) {text("<"); ++p; continue;}
        std::string tag=lower(html.substr(begin,i-begin));
        if(closing) {auto end=html.find('>',i); if(end==std::string::npos) break; for(size_t j=stack.size();j>1;--j) if(stack[j-1]->tag==tag) {stack.resize(j-1); break;} p=end+1; continue;}
        auto node=std::make_unique<Node>(); node->tag=tag; bool self=false;
        while(i<html.size()) {
            while(i<html.size() && space(html[i])) ++i;
            if(i==html.size()) break;
            if(html[i]=='>') {++i;break;}
            if(html[i]=='/' && i+1<html.size() && html[i+1]=='>') {i+=2; self=true;break;}
            auto a=i; while(i<html.size() && nameChar(html[i])) ++i; if(i==a) {++i;continue;}
            auto key=lower(html.substr(a,i-a)); while(i<html.size() && space(html[i])) ++i; std::string value;
            if(i<html.size() && html[i]=='=') {++i; while(i<html.size() && space(html[i])) ++i; if(i<html.size() && (html[i]=='\'' || html[i]=='"')) {char quote=html[i++]; auto end=html.find(quote,i); if(end==std::string::npos) {value=html.substr(i); i=html.size();} else {value=html.substr(i,end-i);i=end+1;}} else {auto v=i; while(i<html.size() && !space(html[i]) && html[i]!='>') ++i;value=html.substr(v,i-v);} }
            node->attributes.emplace(key,decodeEntities(value));
        }
        Node* ptr=node.get(); stack.back()->children.push_back(std::move(node)); ++nodes; p=i;
        if((tag=="script" || tag=="style") && !self) { auto rest=lower(html.substr(p)); auto end=rest.find("</"+tag); if(end==std::string::npos) break; auto close=html.find('>',p+end); if(close==std::string::npos) break; p=close+1; continue; }
        if(!self && !voids.count(tag) && stack.size()<256) stack.push_back(ptr);
    }
    return doc;
}
}
