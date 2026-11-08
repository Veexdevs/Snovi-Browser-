#include "Http.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sslerr.h>
#if !defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_MAJOR < 3
#error "Snovi requires OpenSSL 3.0 or newer"
#endif
#include <openssl/x509v3.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <climits>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstring>
namespace snovi {
namespace {
constexpr size_t max_response=8*1024*1024, max_header=64*1024;
#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle invalidSocket = INVALID_SOCKET;
void closeSocket(SocketHandle fd) { closesocket(fd); }
void ensureSockets() {
    struct Winsock {
        Winsock() { WSADATA data; if (WSAStartup(MAKEWORD(2, 2), &data)!=0) throw std::runtime_error("Falha ao iniciar Winsock"); }
        ~Winsock() { WSACleanup(); }
    };
    static Winsock winsock;
}
// OpenSSL's default file paths do not automatically use Windows' trust store.
void addSystemTrust(SSL_CTX* ctx) {
    HCERTSTORE roots = CertOpenSystemStoreW(0, L"ROOT");
    if(!roots) throw std::runtime_error("Não foi possível acessar os certificados do Windows");
    PCCERT_CONTEXT cert = nullptr;
    while((cert=CertEnumCertificatesInStore(roots, cert)) != nullptr) {
        const unsigned char* data = cert->pbCertEncoded;
        X509* x = d2i_X509(nullptr, &data, cert->cbCertEncoded);
        if(x) { X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), x); X509_free(x); ERR_clear_error(); }
    }
    CertCloseStore(roots, 0);
}
#else
using SocketHandle = int;
constexpr SocketHandle invalidSocket = -1;
void closeSocket(SocketHandle fd) { close(fd); }
void ensureSockets() {}
#endif
struct Socket { SocketHandle fd=invalidSocket; ~Socket(){if(fd!=invalidSocket) closeSocket(fd);} };
std::string lower(std::string s) { std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return std::tolower(c);}); return s; }
std::string trim(std::string s) { auto a=s.find_first_not_of(" \t"), b=s.find_last_not_of(" \t"); return a==std::string::npos ? "" : s.substr(a,b-a+1); }
std::string dechunk(const std::string& data) {
    std::string out; size_t p=0;
    for (;;) {
        auto end=data.find("\r\n",p); if(end==std::string::npos || end-p>32) throw std::runtime_error("Chunk HTTP inválido");
        auto sizeText=data.substr(p,end-p); auto semi=sizeText.find(';'); sizeText=sizeText.substr(0,semi);
        if(sizeText.empty() || !std::all_of(sizeText.begin(),sizeText.end(),[](unsigned char c){return std::isxdigit(c);}) || sizeText.size()>8) throw std::runtime_error("Tamanho de chunk inválido");
        size_t n=std::stoul(sizeText,nullptr,16); p=end+2;
        if(n==0) { if(data.find("\r\n",p)==std::string::npos) throw std::runtime_error("Final de chunk incompleto"); return out; }
        if(n>max_response-out.size() || n>data.size()-p || data.size()-p-n<2 || data.compare(p+n,2,"\r\n")!=0) throw std::runtime_error("Chunk HTTP incompleto");
        out.append(data,p,n); p+=n+2;
    }
}
}
Response HttpClient::request(const Url& url) {
    if(!filter_.allow(url)) throw std::runtime_error("Requisição bloqueada pelo Snovi AdBlock: "+url.host);
    ensureSockets();
    addrinfo hints{}; hints.ai_socktype=SOCK_STREAM; hints.ai_family=AF_UNSPEC; addrinfo* raw=nullptr;
    if(getaddrinfo(url.host.c_str(),url.port.c_str(),&hints,&raw)!=0) throw std::runtime_error("Falha ao resolver o host");
    std::unique_ptr<addrinfo,decltype(&freeaddrinfo)> addresses(raw,freeaddrinfo); Socket socket;
    for(auto* a=raw;a;a=a->ai_next) {
        socket.fd=::socket(a->ai_family,a->ai_socktype,a->ai_protocol); if(socket.fd==invalidSocket) continue;
#ifdef _WIN32
        DWORD timeout=15000; setsockopt(socket.fd,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout)); setsockopt(socket.fd,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
#else
        timeval timeout{15,0}; setsockopt(socket.fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)); setsockopt(socket.fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
#endif
        if(connect(socket.fd,a->ai_addr,a->ai_addrlen)==0) break;
        closeSocket(socket.fd); socket.fd=invalidSocket;
    }
    if(socket.fd==invalidSocket) throw std::runtime_error("Falha na conexão");
    std::unique_ptr<SSL_CTX,decltype(&SSL_CTX_free)> ctx(nullptr,SSL_CTX_free);
    std::unique_ptr<SSL,decltype(&SSL_free)> ssl(nullptr,SSL_free);
    if(url.scheme=="https") {
        ctx.reset(SSL_CTX_new(TLS_client_method())); if(!ctx || SSL_CTX_set_default_verify_paths(ctx.get())!=1) throw std::runtime_error("Não foi possível carregar certificados de confiança");
#ifdef _WIN32
        addSystemTrust(ctx.get());
#endif
        SSL_CTX_set_verify(ctx.get(),SSL_VERIFY_PEER,nullptr); SSL_CTX_set_min_proto_version(ctx.get(),TLS1_2_VERSION);
        ssl.reset(SSL_new(ctx.get())); if(!ssl) throw std::runtime_error("Falha TLS");
        unsigned char ip[16]; bool isIp=inet_pton(AF_INET,url.host.c_str(),ip)==1 || inet_pton(AF_INET6,url.host.c_str(),ip)==1;
        if(isIp) { if(X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl.get()),url.host.c_str())!=1) throw std::runtime_error("Falha na verificação de IP"); }
        else if(SSL_set_tlsext_host_name(ssl.get(),url.host.c_str())!=1 || SSL_set1_host(ssl.get(),url.host.c_str())!=1) throw std::runtime_error("Falha na verificação de host TLS");
#ifdef _WIN32
        if(socket.fd>INT_MAX) throw std::runtime_error("Socket incompatível com OpenSSL");
#endif
        if(SSL_set_fd(ssl.get(),static_cast<int>(socket.fd))!=1) throw std::runtime_error("Falha ao configurar socket TLS");
        if(SSL_connect(ssl.get())!=1 || SSL_get_verify_result(ssl.get())!=X509_V_OK) throw std::runtime_error("Falha TLS ou certificado inválido");
    }
    std::string req="GET "+url.target+" HTTP/1.1\r\nHost: "+url.authority()+"\r\nUser-Agent: Snovi/0.1\r\nAccept: text/html,text/plain;q=0.9,*/*;q=0.1\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
    size_t sent=0; while(sent<req.size()) { int n=ssl ? SSL_write(ssl.get(),req.data()+sent,static_cast<int>(req.size()-sent)) : static_cast<int>(send(socket.fd,req.data()+sent,static_cast<int>(req.size()-sent),
#ifdef _WIN32
        0
#else
        MSG_NOSIGNAL
#endif
        )); if(n<=0) throw std::runtime_error("Falha ao enviar requisição"); sent+=n; }
    std::string data; char buf[8192]; bool uncleanTlsEof=false;
    for(;;) { int n=ssl ? SSL_read(ssl.get(),buf,sizeof(buf)) : static_cast<int>(recv(socket.fd,buf,sizeof(buf),0));
        if(n<=0) { if(ssl) { int e=SSL_get_error(ssl.get(),n); if(e==SSL_ERROR_SYSCALL && n==0) uncleanTlsEof=true; else if(e==SSL_ERROR_SSL && ERR_GET_REASON(ERR_peek_last_error())==SSL_R_UNEXPECTED_EOF_WHILE_READING) { uncleanTlsEof=true; ERR_clear_error(); } else if(e!=SSL_ERROR_ZERO_RETURN) throw std::runtime_error("Falha ao ler resposta TLS"); } else if(n<0) throw std::runtime_error("Falha ao ler resposta"); break; }
        data.append(buf,n); if(data.size()>max_response+max_header) throw std::runtime_error("Resposta excede o limite de 8 MiB");
    }
    auto split=data.find("\r\n\r\n"); if(split==std::string::npos || split>max_header) throw std::runtime_error("Cabeçalho HTTP inválido ou muito grande");
    std::istringstream in(data.substr(0,split)); std::string line, version; Response result{url,0,{}, {}};
    if(!std::getline(in,line)) throw std::runtime_error("Resposta HTTP inválida");
    std::istringstream status(line); status>>version>>result.status;
    if(version.rfind("HTTP/1.",0)!=0 || result.status<100 || result.status>599) throw std::runtime_error("Status HTTP inválido");
    while(std::getline(in,line)) { if(!line.empty() && line.back()=='\r') line.pop_back(); auto c=line.find(':'); if(c==std::string::npos) throw std::runtime_error("Cabeçalho HTTP inválido"); auto key=lower(line.substr(0,c)); if(result.headers.count(key)) result.headers[key]+=","+trim(line.substr(c+1)); else result.headers[key]=trim(line.substr(c+1)); }
    auto body=data.substr(split+4); auto te=result.headers.find("transfer-encoding");
    if(uncleanTlsEof && te==result.headers.end() && !result.headers.count("content-length")) throw std::runtime_error("Resposta TLS sem fechamento seguro e sem tamanho explícito");
    if(te!=result.headers.end()) { if(lower(te->second)!="chunked") throw std::runtime_error("Transfer-Encoding não suportado"); body=dechunk(body); }
    else if(result.headers.count("content-length")) { auto s=result.headers.at("content-length"); if(s.empty() || s.size()>10 || !std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isdigit(c);})) throw std::runtime_error("Content-Length inválido"); auto n=std::stoull(s); if(n>max_response || body.size()!=n) throw std::runtime_error("Corpo HTTP incompleto ou inválido"); }
    if(body.size()>max_response) throw std::runtime_error("Corpo HTTP muito grande");
    result.body=std::move(body); return result;
}
Response HttpClient::get(const Url& initial) {
    Url url=initial;
    for(int i=0;i<=5;++i) { auto r=request(url); if(r.status!=301 && r.status!=302 && r.status!=303 && r.status!=307 && r.status!=308) return r;
        auto it=r.headers.find("location"); if(it==r.headers.end()) throw std::runtime_error("Redirecionamento sem Location");
        if(i==5) throw std::runtime_error("Limite de redirecionamentos atingido");
        auto next=url.resolve(it->second); if(url.scheme=="https" && next.scheme!="https") throw std::runtime_error("Redirecionamento HTTPS para HTTP bloqueado"); url=next;
    }
    throw std::runtime_error("Limite de redirecionamentos atingido");
}
}
