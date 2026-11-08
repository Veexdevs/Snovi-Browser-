# Snovi Browser

**Snovi — Browse beyond.**

O Snovi é um projeto experimental de navegador independente. O motor de rede, parser HTML, DOM e renderizador textual são escritos em C++; o TLS usa OpenSSL. A interface desktop é feita com Qt Widgets (PySide6), sem Qt WebEngine, Chromium, Blink, Gecko, WebKit, Electron ou WebView. A aparência da barra de abas e navegação busca referências em Firefox e Brave, com a identidade azul do Snovi.

> **Estado atual:** há uma janela gráfica funcional no Linux, mas o conteúdo dos sites ainda é exibido como texto e links separados. Isto **não é um navegador completo**, não tem compatibilidade com a Web moderna nem é adequado para navegação sensível. A automação de `.exe` é experimental e ainda precisa ser validada em um runner Windows; não existem `.apk` ou `.ipa` neste projeto.

## Executar a interface gráfica no Linux

São necessários Linux, CMake 3.16+, compilador C++17, OpenSSL 3 com arquivos de desenvolvimento, Python 3 com `venv`, acesso a um ambiente gráfico e bibliotecas de sistema exigidas pelo Qt. No Debian/Ubuntu, instale, por exemplo:

```sh
sudo apt install build-essential cmake libssl-dev python3 python3-venv libgl1 libegl1 libxkbcommon0 libdbus-1-3
cmake -S Snovi -B build
cmake --build build
python3 -m venv .venv
.venv/bin/pip install -r Snovi/UI/Desktop/requirements.txt
./Snovi/UI/Desktop/run.sh
# Opcional: abrir uma página ao iniciar
./Snovi/UI/Desktop/run.sh https://example.com/
```

A biblioteca `build/libsnovi_bridge.so` conecta a janela ao motor real; se usar outro diretório de compilação, configure `SNOVI_ENGINE_LIBRARY` com o caminho absoluto da biblioteca. O Qt no Linux também pode exigir dependências adicionais do plugin de plataforma XCB, conforme a distribuição. O executável textual original continua disponível como `./build/snovi`.

A interface oferece abas, navegação, atualização, página inicial, favoritos e histórico por sessão, pesquisa via DuckDuckGo, links clicáveis, cópia do endereço, código-fonte somente leitura e um menu ⋮. O carregamento de rede ocorre fora da thread da interface; durante uma requisição os controles de navegação ficam desabilitados. A pesquisa abre uma URL de pesquisa real, mas a página de resultados pode não funcionar corretamente pelas limitações do motor. Favoritos e histórico não são persistidos; o botão de início reinicia a aba atual. O zoom altera o tamanho e a disposição do texto, não um layout gráfico completo. Não há cache, de modo que atualizar sempre faz uma nova requisição.

### Filtro de rede experimental

No menu, ative/desative o filtro ou carregue uma lista local. O formato é **um domínio por linha**, com comentários iniciados por `#`; não é o formato EasyList/Adblock Plus. Exemplo:

```text
# Lista local de teste
ads.example.com
```

Domínios e subdomínios correspondentes são bloqueados antes de cada requisição, inclusive redirecionamentos. A lista começa vazia e não é persistida nem atualizada automaticamente. O contador mede requisições bloqueadas nesta sessão, não anúncios detectados. Como o motor ainda não baixa sub-recursos nem executa scripts, ele **não bloqueia anúncios de páginas modernas**. Não há bloqueio cosmético nem gerenciamento persistente de listas/exceções. A interface textual permite também adicionar exceções durante a sessão com `permitir dominio`.

## Testes

```sh
ctest --test-dir build --output-on-failure
# Teste opcional de widgets Qt, em ambiente sem tela:
QT_QPA_PLATFORM=offscreen .venv/bin/python Snovi/tests/desktop_test.py "$PWD/build/libsnovi_bridge.so"
```

Os testes padrão cobrem componentes, ponte da interface ao motor e rede local, incluindo rejeição de certificados inválidos. O teste opcional exercita a janela, navegação, links, favoritos, abas, zoom e cópia do endereço. Para incluí-lo no CTest, configure CMake com `-DSNOVI_TEST_DESKTOP=ON` e `-DPython3_EXECUTABLE="$PWD/.venv/bin/python"`.

## GitHub Actions e artefatos

- **Windows:** o workflow `.github/workflows/windows.yml` compila o motor com MSVC e OpenSSL, executa testes, empacota a interface Qt com PyInstaller e testa a abertura do próprio `Snovi.exe`. Em caso de sucesso, o artefato `Snovi-Windows-x64-experimental` fica disponível na execução do Actions por 14 dias. O executável não é assinado nem é um instalador. Não há publicação automática em Releases. Após enviar as alterações ao GitHub, o fluxo roda em pull requests e em pushes para `main`; também pode ser iniciado manualmente na aba Actions quando o workflow estiver disponível no repositório.
- **Linux:** `.github/workflows/linux.yml` compila e testa o motor e a interface, sem empacotar um instalador.
- **Android/iOS:** ainda não há workflows de `.apk` ou `.ipa`: a interface PySide6 atual é desktop e ainda faltam interfaces móveis, integração do motor e testes nessas plataformas. Um `.ipa` instalável também exige assinatura e provisionamento Apple apropriados; gerar um arquivo vazio ou não instalável não resolve esses requisitos.

O código de rede foi adaptado para Winsock e importa certificados raiz da loja ROOT do Windows para o OpenSSL. Isso não implementa todas as políticas de confiança/revogação do Windows; a versão continua experimental e não deve ser usada para navegação sensível. O build Windows deve ser confirmado no runner do GitHub Actions antes de ser considerado funcional.

## Limitações e plataformas

- **Rede:** GET HTTP/1.1 em sockets POSIX ou Winsock, HTTPS com verificação de cadeia e nome/IP e TLS 1.2+, redirecionamentos limitados sem downgrade HTTPS para HTTP e limite de resposta de 8 MiB. Sem HTTP/2, proxy, compressão, cache, cookies, formulários ou downloads. DNS e conexão são síncronos no motor; o timeout de conexão não é garantido. Erros de certificado não são ignorados.
- **HTML/DOM:** parser próprio simplificado com elementos, atributos, texto, algumas entidades, links e texto alternativo de imagens. Não implementa o algoritmo HTML5, codificações de caracteres, formulários funcionais ou carregamento de imagens. Muitos sites são interpretados incorretamente.
- **Renderização:** texto e links, não o layout gráfico original do site. Não há renderizador OpenGL de páginas, CSS, imagens ou JavaScript. Código-fonte é mostrado sem realce de sintaxe.
- **Segurança:** não há política de mesma origem, isolamento de processos, permissões, proteção contra malware ou armazenamento seguro. Não use para navegação sensível. O filtro de rede não substitui essas proteções.
- **Distribuição:** a base Linux foi testada localmente; a adaptação Windows e o `.exe` ainda precisam passar pelo runner do GitHub. `.apk` para Android e `.ipa` para iOS exigem interfaces móveis, integração nativa, compilação e testes nessas plataformas. Ainda não há instaladores ou aplicativos móveis distribuíveis; o código atual não produz um aplicativo iOS. O termo `.opa` não identifica um formato de pacote conhecido neste projeto.

O código está organizado em `Snovi/Network`, `HTML`, `Renderer`, `AdBlock`, `Browser`, `Platform` (ponte C) e `UI/Desktop` (janela). Os componentes e plataformas restantes devem ser adicionados progressivamente com testes e modelos de segurança próprios.
