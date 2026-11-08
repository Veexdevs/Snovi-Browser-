# Snovi Browser

**Snovi — Browse beyond.**

Este repositório contém o primeiro incremento experimental de um navegador independente. O código de rede, parser HTML, DOM e renderizador textual é próprio; não usa Chromium, Blink, Gecko, WebKit, CEF, Electron ou WebView. OpenSSL é usado apenas para TLS. A imagem fornecida para a identidade visual está em `Snovi/UI/Home/assets/snovi-logo.png`.

> **Ainda não é um navegador gráfico nem um produto seguro para uso diário.** O programa atual é uma interface de terminal para desenvolver e testar o motor. Não possui a interface responsiva, o menu gráfico de três pontos ou a compatibilidade com sites modernos solicitados. Não há recursos simulados para preencher essas lacunas.

## Compilar e testar no Linux

Dependências: compilador C++17, CMake 3.16 ou superior, headers e bibliotecas de desenvolvimento OpenSSL 3.0 ou superior, e Python 3 para o teste de integração. Por exemplo, no Debian/Ubuntu: `sudo apt install build-essential cmake libssl-dev python3`.

```sh
cmake -S Snovi -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/snovi https://example.com/
```

Digite `ajuda` para a lista de comandos, ou `sair` para terminar. Exemplos: `abrir http://localhost:8000/`, `link 1`, `fonte`, `voltar`, `avancar`, `atualizar`, `aba nova`, `aba 1`, `zoom +`, `favorito` e `historico`. `fonte` mostra o corpo recebido, após decodificação HTTP chunked, sem executar o conteúdo; caracteres de controle perigosos para o terminal são removidos da exibição. `atualizar` faz uma nova requisição (não há cache). O zoom altera a largura do texto no terminal, não a escala gráfica de uma página. Abas, histórico, favoritos e exceções existem somente na sessão; `inicio` reinicia a aba atual.

### Filtro de requisições experimental

`lista caminho/arquivo.txt` substitui a lista em memória. O formato é **um domínio por linha**, com comentários iniciados por `#`; não é o formato EasyList/Adblock Plus. Por exemplo:

```text
# Lista local de teste
ads.example.com
```

`adblock status`, `adblock on`, `adblock off` e `permitir ads.example.com` controlam o filtro. Domínios e subdomínios correspondentes são bloqueados antes de cada requisição, inclusive redirecionamentos. A lista começa vazia; o projeto não fornece nem atualiza uma lista de anúncios. O contador mede requisições bloqueadas nesta sessão, não anúncios detectados. Como o motor ainda não baixa sub-recursos nem executa scripts, ele **não bloqueia anúncios de páginas modernas**. Não há bloqueio cosmético nem gerenciamento persistente de listas/exceções.

## Estado e limitações

- **Rede:** GET HTTP/1.1 sobre sockets POSIX; HTTPS com OpenSSL, verificação de cadeia e nome/IP, TLS 1.2 ou superior; redirecionamentos limitados e sem downgrade HTTPS para HTTP; decodificação chunked e limite de resposta de 8 MiB. Sem HTTP/2, proxy, compressão, streaming, cache, cookies, uploads ou downloads. DNS e conexão são síncronos; o timeout de conexão não é garantido. Erros de certificado não são ignorados.
- **HTML/DOM:** parser próprio simplificado com elementos, atributos, texto, algumas entidades, links e texto alternativo de imagens. Não implementa o algoritmo HTML5, codificações de caracteres, formulários funcionais ou carregamento de imagens. Páginas malformadas e muitos sites reais podem ser interpretados incorretamente.
- **Renderização:** somente texto no terminal e links numerados; não há OpenGL, layout visual, imagens, rolagem gráfica ou CSS. O zoom do terminal não corresponde ao zoom de um navegador gráfico.
- **JavaScript e CSS:** ainda não implementados. Scripts e estilos não são executados.
- **Segurança:** não há política de mesma origem, isolamento de processos, permissões, proteção contra malware ou armazenamento seguro. Não use para navegação sensível. O filtro de rede não substitui essas proteções.
- **Plataformas:** implementação de rede POSIX testada em Linux; Android, Windows e macOS ainda não são suportados. Compartilhamento nativo, cópia para área de transferência, persistência, configurações gráficas e gerenciador de downloads ainda faltam.
- **Interface:** não há barra gráfica, menu ⋮, realce de sintaxe, pesquisa ou atalhos personalizáveis. A interface de terminal não pretende substituir a interface final.

O código está separado por responsabilidades em `Snovi/Network`, `HTML`, `Renderer`, `AdBlock`, `Browser` e `UI/Menu`. Os demais componentes e plataformas devem ser adicionados progressivamente, com testes e modelos de segurança próprios, antes de se considerar o projeto um navegador completo.
