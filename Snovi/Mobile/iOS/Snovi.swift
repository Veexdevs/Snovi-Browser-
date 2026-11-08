import UIKit
import Foundation
import Darwin

@_silgen_name("snovi_create") private func snoviCreate() -> UnsafeMutableRawPointer?
@_silgen_name("snovi_destroy") private func snoviDestroy(_ handle: UnsafeMutableRawPointer)
@_silgen_name("snovi_execute") private func snoviExecute(_ handle: UnsafeMutableRawPointer, _ action: UnsafePointer<CChar>, _ value: UnsafePointer<CChar>) -> UnsafeMutablePointer<CChar>?
@_silgen_name("snovi_free") private func snoviFree(_ value: UnsafeMutablePointer<CChar>)

@main final class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?
    func application(_ application: UIApplication, didFinishLaunchingWithOptions options: [UIApplication.LaunchOptionsKey: Any]? = nil) -> Bool {
        let window = UIWindow(frame: UIScreen.main.bounds)
        window.rootViewController = BrowserController()
        window.makeKeyAndVisible(); self.window = window
        return true
    }
}

final class BrowserController: UIViewController, UITextFieldDelegate {
    private let queue = DispatchQueue(label: "snovi.engine")
    private var engine: UnsafeMutableRawPointer?
    private var state: [String: Any] = [:]
    private var busy = false
    private let blue = UIColor(red: 0.12, green: 0.40, blue: 0.83, alpha: 1)
    private let address = UITextField(), text = UITextView(), status = UILabel(), linkStack = UIStackView(), progress = UIActivityIndicatorView(style: .medium)
    private let controls = UIStackView()

    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = UIColor(red: 0.95, green: 0.97, blue: 0.99, alpha: 1)
        build()
        guard let ca = Bundle.main.path(forResource: "ca-bundle", ofType: "pem"), setenv("SNOVI_CA_FILE", ca, 1) == 0, let ptr = snoviCreate() else {
            text.text = "Não foi possível iniciar o motor ou carregar os certificados."; return
        }
        engine = ptr
        perform("state")
    }
    deinit { if let ptr = engine { snoviDestroy(ptr) } }
    private func button(_ title: String, _ selector: Selector) -> UIButton {
        let b = UIButton(type: .system); b.setTitle(title, for: .normal); b.setTitleColor(blue, for: .normal); b.addTarget(self, action: selector, for: .touchUpInside); return b
    }
    private func build() {
        let root = UIStackView(); root.axis = .vertical; root.spacing = 8; root.translatesAutoresizingMaskIntoConstraints = false; view.addSubview(root)
        NSLayoutConstraint.activate([root.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 8), root.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor, constant: 12), root.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor, constant: -12), root.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -8)])
        let brand = UILabel(); brand.text = "SNOVI  /  BROWSE BEYOND"; brand.textColor = blue; brand.font = .boldSystemFont(ofSize: 13); root.addArrangedSubview(brand)
        controls.axis = .horizontal; controls.distribution = .fillEqually
        controls.addArrangedSubview(button("‹", #selector(back))); controls.addArrangedSubview(button("›", #selector(forward))); controls.addArrangedSubview(button("⟳", #selector(reload))); controls.addArrangedSubview(button("⌂", #selector(home))); controls.addArrangedSubview(button("Abas", #selector(tabs))); controls.addArrangedSubview(button("⋮", #selector(menu))); root.addArrangedSubview(controls)
        address.borderStyle = .roundedRect; address.placeholder = "Pesquisar ou digitar endereço"; address.returnKeyType = .go; address.autocapitalizationType = .none; address.autocorrectionType = .no; address.delegate = self; root.addArrangedSubview(address)
        progress.hidesWhenStopped = true; root.addArrangedSubview(progress)
        text.isEditable = false; text.isSelectable = true; text.backgroundColor = .white; text.textColor = .darkText; text.layer.cornerRadius = 10; root.addArrangedSubview(text)
        let links = UIScrollView(); links.translatesAutoresizingMaskIntoConstraints = false; links.heightAnchor.constraint(equalToConstant: 110).isActive = true; root.addArrangedSubview(links)
        linkStack.axis = .vertical; linkStack.alignment = .fill; linkStack.translatesAutoresizingMaskIntoConstraints = false; links.addSubview(linkStack)
        NSLayoutConstraint.activate([linkStack.topAnchor.constraint(equalTo: links.contentLayoutGuide.topAnchor), linkStack.bottomAnchor.constraint(equalTo: links.contentLayoutGuide.bottomAnchor), linkStack.leadingAnchor.constraint(equalTo: links.contentLayoutGuide.leadingAnchor), linkStack.trailingAnchor.constraint(equalTo: links.contentLayoutGuide.trailingAnchor), linkStack.widthAnchor.constraint(equalTo: links.frameLayoutGuide.widthAnchor)])
        status.numberOfLines = 2; status.font = .systemFont(ofSize: 10); status.textColor = .gray; root.addArrangedSubview(status)
    }
    func textFieldShouldReturn(_ field: UITextField) -> Bool { field.resignFirstResponder(); navigate(field.text ?? ""); return true }
    private func navigate(_ input: String) {
        var url = input.trimmingCharacters(in: .whitespacesAndNewlines); if url.isEmpty { return }
        if !url.contains("://") {
            if !url.contains(" ") && (url.contains(".") || url.hasPrefix("localhost")) { url = "https://" + url }
            else { var c = URLComponents(string: "https://duckduckgo.com/")!; c.queryItems = [URLQueryItem(name: "q", value: url)]; url = c.string ?? "" }
        }
        perform("navigate", url)
    }
    private func perform(_ action: String, _ value: String = "") {
        guard !busy, let ptr = engine else { return }
        busy = true; controls.isUserInteractionEnabled = false; address.isEnabled = false; progress.startAnimating()
        queue.async { [self] in
            let raw = action.withCString { a in value.withCString { v in snoviExecute(ptr, a, v) } }
            var result: [String: Any]? = nil
            if let raw = raw {
                let json = String(decodingCString: raw, as: UTF8.self); snoviFree(raw)
                if let data = json.data(using: .utf8) { result = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any] }
            }
            DispatchQueue.main.async { [self] in
                busy = false; controls.isUserInteractionEnabled = true; address.isEnabled = true; progress.stopAnimating()
                guard let result = result else { alert("Falha no motor Snovi"); return }
                state = result; render()
                if let error = result["error"] as? String, !error.isEmpty { alert(error) }
            }
        }
    }
    private var url: String { state["url"] as? String ?? "" }
    private var zoom: Int { state["zoom"] as? Int ?? 100 }
    private func render() {
        address.text = url
        text.text = url.isEmpty ? "SNOVI\nBrowse beyond.\n\nDigite um endereço para começar.\n\nA renderização é textual nesta versão experimental." : (state["text"] as? String ?? "")
        text.font = .systemFont(ofSize: CGFloat(16 * zoom) / 100)
        linkStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        let links = state["links"] as? [String] ?? []
        for (index, href) in links.prefix(100).enumerated() {
            let b = UIButton(type: .system); b.contentHorizontalAlignment = .leading; b.setTitle("\(index+1)  \(href)", for: .normal); b.tag = index; b.addTarget(self, action: #selector(openLink(_:)), for: .touchUpInside); linkStack.addArrangedSubview(b)
        }
        if links.count > 100 { let label = UILabel(); label.text = "Mostrando os primeiros 100 links"; linkStack.addArrangedSubview(label) }
        status.text = "MOTOR EXPERIMENTAL · Modo texto · Zoom \(zoom)% · \(state["blocked"] as? Int ?? 0) requisições bloqueadas"
    }
    @objc private func openLink(_ sender: UIButton) { perform("link", String(sender.tag)) }
    @objc private func back() { perform("back") }
    @objc private func forward() { perform("forward") }
    @objc private func reload() { perform("reload") }
    @objc private func home() { perform("home") }
    private func alert(_ message: String) { let a = UIAlertController(title: "Snovi", message: message, preferredStyle: .alert); a.addAction(UIAlertAction(title: "OK", style: .cancel)); present(a, animated: true) }
    @objc private func tabs() {
        let a = UIAlertController(title: "Abas", message: nil, preferredStyle: .alert)
        let all = state["tabs"] as? [Any] ?? []
        for (i, item) in all.enumerated() { a.addAction(UIAlertAction(title: item as? String ?? "Nova aba", style: .default) { _ in self.perform("select_tab", String(i)) }) }
        a.addAction(UIAlertAction(title: "＋ Nova aba", style: .default) { _ in self.perform("new_tab") })
        a.addAction(UIAlertAction(title: "Fechar aba atual", style: .destructive) { _ in self.perform("close_tab", String(self.state["currentTab"] as? Int ?? 0)) })
        a.addAction(UIAlertAction(title: "Cancelar", style: .cancel)); present(a, animated: true)
    }
    private func showList(_ title: String, _ key: String) {
        let values = state[key] as? [String] ?? []
        guard !values.isEmpty else { alert("Nenhuma página nesta sessão."); return }
        let a = UIAlertController(title: title, message: nil, preferredStyle: .alert)
        for u in values { a.addAction(UIAlertAction(title: u, style: .default) { _ in self.navigate(u) }) }
        a.addAction(UIAlertAction(title: "Fechar", style: .cancel)); present(a, animated: true)
    }
    private func showSource() {
        let screen = UIViewController(); screen.view.backgroundColor = .white
        let box = UITextView(); box.isEditable = false; box.isSelectable = true; box.font = .monospacedSystemFont(ofSize: 12, weight: .regular); box.text = url + "\n\n" + (state["source"] as? String ?? ""); box.translatesAutoresizingMaskIntoConstraints = false; screen.view.addSubview(box)
        NSLayoutConstraint.activate([box.topAnchor.constraint(equalTo: screen.view.safeAreaLayoutGuide.topAnchor), box.bottomAnchor.constraint(equalTo: screen.view.safeAreaLayoutGuide.bottomAnchor), box.leadingAnchor.constraint(equalTo: screen.view.leadingAnchor), box.trailingAnchor.constraint(equalTo: screen.view.trailingAnchor)])
        present(screen, animated: true)
    }
    @objc private func menu() {
        let a = UIAlertController(title: "Menu Snovi", message: nil, preferredStyle: .alert)
        func add(_ title: String, _ handler: @escaping () -> Void) { a.addAction(UIAlertAction(title: title, style: .default) { _ in handler() }) }
        add("Nova aba") { self.perform("new_tab") }; add("Atualizar página") { self.reload() }
        if !url.isEmpty {
            add("Ver código-fonte") { self.showSource() }
            add("Compartilhar") { let share = UIActivityViewController(activityItems: [self.url], applicationActivities: nil); share.popoverPresentationController?.sourceView = self.view; share.popoverPresentationController?.sourceRect = CGRect(x: self.view.bounds.midX, y: self.view.bounds.midY, width: 1, height: 1); self.present(share, animated: true) }
            add("Copiar endereço") { UIPasteboard.general.string = self.url; self.alert("Endereço copiado") }
            add("Adicionar/remover favorito") { self.perform("favorite") }
        }
        add("Zoom +") { self.perform("zoom", String(self.zoom+10)) }; add("Zoom −") { self.perform("zoom", String(self.zoom-10)) }; add("Restaurar zoom") { self.perform("zoom", "100") }
        add("Favoritos") { self.showList("Favoritos", "favorites") }; add("Histórico") { self.showList("Histórico", "history") }
        add("Ativar/desativar filtro") { self.perform("adblock") }
        add("Sobre") { self.alert("Snovi experimental. Conteúdo em texto; sem CSS, JavaScript ou imagens. O filtro começa vazio e não bloqueia anúncios de sites modernos.") }
        a.addAction(UIAlertAction(title: "Fechar", style: .cancel)); present(a, animated: true)
    }
}
