# Snovi Browser

**Snovi — Browse beyond.**

O Snovi é um projeto experimental de navegador independente. O motor de rede, parser HTML, DOM e renderizador textual são escritos em C++; o TLS usa OpenSSL. A interface desktop é feita com Qt Widgets (PySide6), sem Qt WebEngine, Chromium, Blink, Gecko, WebKit, Electron ou WebView. A aparência da barra de abas e navegação busca referências em Firefox e Brave, com a identidade azul do Snovi.

> **Estado atual:** há uma janela gráfica funcional no Linux, mas o conteúdo dos sites ainda é exibido como texto e links separados. Isto **não é um navegador completo**, não tem compatibilidade com a Web moderna nem é adequado para navegação sensível. Os builds móveis e o `.exe` também são experimentais e ainda precisam ser validados nos runners correspondentes; os artefatos não são gerados neste checkout automaticamente.

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
- **Android/iOS:** `.github/workflows/mobile.yml` foi preparado para compilar o motor C++ e as interfaces nativas (Android Java/JNI e iOS Swift/UIKit), sem WebView. O job Android gera um APK **debug** assinado com uma chave de desenvolvimento do runner, verifica o pacote e tenta instalar/navegar em um emulador. O job iOS compila um `.app` arm64 e o empacota como IPA **não assinado**. Se os jobs passarem, seus artefatos ficam disponíveis na execução do Actions por 14 dias. **O IPA não é instalável como está**: para instalar ou distribuir é necessário assiná-lo novamente com certificado e perfil de provisionamento Apple apropriados. Não há publicação automática em lojas ou Releases. Esses jobs e as interfaces móveis ainda não foram executados nem validados neste ambiente; não há garantia de que o primeiro build no GitHub terá sucesso.

O código de rede foi adaptado para Winsock e importa certificados raiz da loja ROOT do Windows para o OpenSSL. Isso não implementa todas as políticas de confiança/revogação do Windows; a versão continua experimental e não deve ser usada para navegação sensível. O build Windows deve ser confirmado no runner do GitHub Actions antes de ser considerado funcional.

### Builds móveis e confiança TLS

O fluxo móvel baixa e compila OpenSSL 3.5.9 e inclui o pacote de certificados CA do `certifi` 2026.10.23. O motor valida cadeia e nome do host; não há opção de ignorar erros de certificado. As interfaces móveis usam essa lista empacotada, **não** a loja de certificados do Android/iOS; certificados instalados pelo usuário não são automaticamente reconhecidos, nem todas as políticas do sistema e verificações de revogação são aplicadas. Atualizar a lista e distribuir uma nova versão é responsabilidade do projeto. Sem o arquivo CA, a conexão HTTPS falha. HTTP sem criptografia também é permitido quando explicitamente solicitado.

Para testar a automação após enviar as alterações ao GitHub, abra **Actions → Mobile experimental builds** e selecione a execução correspondente ao push para `main` ou ao pull request; `workflow_dispatch` também permite execução manual quando o workflow estiver disponível no repositório. Baixe os artefatos apenas se os jobs terminarem com sucesso. O APK debug é destinado a testes, não é um pacote de produção assinado para a Play Store; a chave de debug do runner é efêmera e builds futuros podem não atualizar uma instalação anterior. O IPA não assinado exige assinatura/provisionamento antes de qualquer instalação.

Para compilação local, use o workflow como referência: Android requer SDK 35, NDK 27.2.12479018, CMake 3.22.1, Java 17 e Gradle 8.10.2; execute `Snovi/Mobile/scripts/android-openssl.sh` com `ANDROID_NDK_ROOT` definido, copie `certifi.where()` para `Snovi/Mobile/Android/app/src/main/assets/ca-bundle.pem` e execute `gradle assembleDebug` em `Snovi/Mobile/Android`. iOS requer macOS, Xcode com SDK iPhoneOS e CMake; copie o mesmo bundle para `Snovi/Mobile/iOS/ca-bundle.pem` e execute `Snovi/Mobile/scripts/ios-build.sh`. Os scripts baixam OpenSSL da tag indicada. Nenhuma credencial de assinatura é armazenada no projeto.

## Limitações e plataformas

- **Rede:** GET HTTP/1.1 em sockets POSIX ou Winsock, HTTPS com verificação de cadeia e nome/IP e TLS 1.2+, redirecionamentos limitados sem downgrade HTTPS para HTTP e limite de resposta de 8 MiB. Sem HTTP/2, proxy, compressão, cache, cookies, formulários ou downloads. DNS e conexão são síncronos no motor; o timeout de conexão não é garantido. Erros de certificado não são ignorados.
- **HTML/DOM:** parser próprio simplificado com elementos, atributos, texto, algumas entidades, links e texto alternativo de imagens. Não implementa o algoritmo HTML5, codificações de caracteres, formulários funcionais ou carregamento de imagens. Muitos sites são interpretados incorretamente.
- **Renderização:** texto e links, não o layout gráfico original do site. Não há renderizador OpenGL de páginas, CSS, imagens ou JavaScript. Código-fonte é mostrado sem realce de sintaxe.
- **Segurança:** não há política de mesma origem, isolamento de processos, permissões, proteção contra malware ou armazenamento seguro. Não use para navegação sensível. O filtro de rede não substitui essas proteções.
- **Distribuição:** a base Linux foi testada localmente; a adaptação Windows, o `.exe` e os builds móveis ainda precisam passar pelos runners do GitHub e por testes em dispositivos reais. O APK é de desenvolvimento e o IPA é não assinado. Não há instaladores nem aplicativos prontos para publicação em lojas.

O código está organizado em `Snovi/Network`, `HTML`, `Renderer`, `AdBlock`, `Browser`, `Platform` (ponte C/JNI), `UI/Desktop` (janela) e `Mobile` (interfaces nativas e scripts). Os componentes e plataformas restantes devem ser adicionados progressivamente com testes e modelos de segurança próprios.
