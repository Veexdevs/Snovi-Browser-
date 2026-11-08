"""Snovi native desktop shell. Qt Widgets only; all fetching/parsing is in Snovi C++."""
import os
import sys
from pathlib import Path
from urllib.parse import quote_plus, urlsplit

from PySide6.QtCore import Qt, QThread, Signal, QTimer
from PySide6.QtGui import QFont, QIcon, QPixmap
from PySide6.QtWidgets import (QApplication, QDialog, QFileDialog, QFrame, QHBoxLayout,
    QLabel, QLineEdit, QListWidget, QMainWindow, QMenu, QMessageBox, QPlainTextEdit,
    QProgressBar, QPushButton, QScrollArea, QStackedWidget, QTabBar, QToolButton,
    QVBoxLayout, QWidget)

from engine import Engine
from styles import STYLES

LOGO = (Path(sys._MEIPASS) / "assets/snovi-logo.png" if getattr(sys, "frozen", False)
        else Path(__file__).resolve().parents[3] / "Snovi/UI/Home/assets/snovi-logo.png")


def destination(value):
    value = value.strip()
    if not value:
        return ""
    if value.startswith(("http://", "https://")):
        return value
    # Explicit, unsupported schemes are left for the engine to reject, not searched.
    if "://" in value:
        return value
    if " " not in value and ("." in value.split('/')[0] or value.startswith("localhost") or value.startswith("[::1]")):
        return "https://" + value
    return "https://duckduckgo.com/?q=" + quote_plus(value)


def label(text, name=None):
    item = QLabel(text)
    item.setTextFormat(Qt.TextFormat.PlainText)
    if name:
        item.setObjectName(name)
    return item


def tool(text, tip, callback):
    button = QToolButton()
    button.setText(text)
    button.setToolTip(tip)
    button.setAccessibleName(tip)
    button.clicked.connect(callback)
    return button


class Worker(QThread):
    result = Signal(dict)
    def __init__(self, engine, action, value):
        super().__init__()
        self.engine, self.action, self.value = engine, action, value
    def run(self):
        try:
            self.result.emit(self.engine.execute(self.action, self.value))
        except Exception as exc:
            self.result.emit({"fatal": str(exc)})


class BrowserWindow(QMainWindow):
    def __init__(self, engine):
        super().__init__()
        self.engine = engine
        self.data = engine.execute("state")
        self.worker = None
        self.setWindowTitle("Snovi Browser")
        self.setWindowIcon(QIcon(str(LOGO)))
        self.resize(1200, 800)
        self.setMinimumSize(640, 480)
        self.build()
        self.refresh()

    def build(self):
        outer = QWidget(objectName="window")
        self.setCentralWidget(outer)
        root = QVBoxLayout(outer); root.setContentsMargins(0, 0, 0, 0); root.setSpacing(0)
        self.top = QFrame(objectName="top")
        head = QVBoxLayout(self.top); head.setContentsMargins(14, 0, 14, 10); head.setSpacing(0)
        tabrow = QHBoxLayout(); tabrow.setContentsMargins(0, 0, 0, 0); tabrow.setSpacing(3)
        self.tabs = QTabBar(); self.tabs.setExpanding(False); self.tabs.setMovable(False); self.tabs.setTabsClosable(True)
        self.tabs.currentChanged.connect(lambda i: self.run_action("select_tab", str(i)) if i >= 0 else None)
        self.tabs.tabCloseRequested.connect(lambda i: self.run_action("close_tab", str(i)))
        tabrow.addWidget(self.tabs)
        add = tool("+", "Nova aba", lambda: self.run_action("new_tab")); add.setObjectName("newTab"); tabrow.addWidget(add)
        tabrow.addStretch()
        badge = label("SNOVI   /   BROWSE BEYOND", "eyebrow"); tabrow.addWidget(badge)
        head.addLayout(tabrow)
        toolbar = QFrame(objectName="toolbar")
        row = QHBoxLayout(toolbar); row.setContentsMargins(5, 4, 5, 4); row.setSpacing(3)
        self.back = tool("‹", "Voltar", lambda: self.run_action("back")); row.addWidget(self.back)
        self.forward = tool("›", "Avançar", lambda: self.run_action("forward")); row.addWidget(self.forward)
        self.reload = tool("⟳", "Atualizar página", lambda: self.run_action("reload")); row.addWidget(self.reload)
        row.addWidget(tool("⌂", "Página inicial", lambda: self.run_action("home")))
        self.address = QLineEdit(); self.address.setPlaceholderText("Pesquisar ou digitar um endereço"); self.address.setAccessibleName("Barra de endereço")
        self.address.returnPressed.connect(lambda: self.navigate(self.address.text())); row.addWidget(self.address, 1)
        self.star = tool("☆", "Adicionar ou remover favorito", lambda: self.run_action("favorite")); row.addWidget(self.star)
        menu_button = tool("⋮", "Menu do Snovi", self.open_menu); menu_button.setObjectName("menuButton"); row.addWidget(menu_button)
        self.menu_button = menu_button
        head.addWidget(toolbar); root.addWidget(self.top)
        self.progress = QProgressBar(); self.progress.setTextVisible(False); self.progress.hide(); root.addWidget(self.progress)
        self.stack = QStackedWidget(); root.addWidget(self.stack, 1)
        self.home = self.build_home(); self.stack.addWidget(self.home)
        self.page_widget = self.build_page(); self.stack.addWidget(self.page_widget)
        self.status = label("MOTOR SNOVI EXPERIMENTAL   ·   Conteúdo exibido em modo texto", "status"); root.addWidget(self.status)

    def build_home(self):
        scroll = QScrollArea(); scroll.setWidgetResizable(True); scroll.setObjectName("home")
        body = QWidget(objectName="home"); scroll.setWidget(body)
        wrap = QVBoxLayout(body); wrap.setContentsMargins(40, 35, 40, 45); wrap.setSpacing(0)
        wrap.addStretch(1)
        center = QWidget(); center.setMaximumWidth(760)
        col = QVBoxLayout(center); col.setContentsMargins(0, 0, 0, 0); col.setSpacing(0)
        brand = QHBoxLayout(); brand.setAlignment(Qt.AlignmentFlag.AlignCenter); brand.setSpacing(12)
        mark = QLabel(); mark.setPixmap(QPixmap(str(LOGO)).scaled(56, 56, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)); brand.addWidget(mark)
        brandcol = QVBoxLayout(); brandcol.setSpacing(1); brandcol.addWidget(label("SNOVI", "title")); brandcol.addWidget(label("BROWSE BEYOND.", "eyebrow")); brand.addLayout(brandcol); col.addLayout(brand)
        col.addSpacing(20)
        intro = label("Seu ponto de partida para explorar a web.", "subtitle"); intro.setAlignment(Qt.AlignmentFlag.AlignCenter); col.addWidget(intro)
        col.addSpacing(27)
        self.search = QLineEdit(objectName="search"); self.search.setPlaceholderText("⌕    Pesquise na web ou digite um endereço"); self.search.setAccessibleName("Pesquisar na web"); self.search.returnPressed.connect(lambda: self.navigate(self.search.text())); col.addWidget(self.search)
        col.addSpacing(36)
        row = QHBoxLayout(); row.addWidget(label("Acesso rápido", "sectionTitle")); row.addStretch(); row.addWidget(label("SEUS ATALHOS", "sectionNote")); col.addLayout(row); col.addSpacing(13)
        shortcuts = QHBoxLayout(); shortcuts.setSpacing(12)
        for title, domain, url in [("Wikipedia", "wikipedia.org", "https://www.wikipedia.org/"), ("GitHub", "github.com", "https://github.com/"), ("DuckDuckGo", "duckduckgo.com", "https://duckduckgo.com/")]:
            btn = QPushButton(title + "\n" + domain, objectName="shortcut"); btn.clicked.connect(lambda _=False, u=url: self.navigate(u)); shortcuts.addWidget(btn)
        col.addLayout(shortcuts); col.addSpacing(31)
        row = QHBoxLayout(); row.addWidget(label("Sua navegação", "sectionTitle")); row.addStretch(); row.addWidget(label("NESTA SESSÃO", "sectionNote")); col.addLayout(row); col.addSpacing(13)
        cards = QHBoxLayout(); cards.setSpacing(12)
        self.favorite_list = self.home_card(cards, "☆", "Favoritos", "Suas páginas salvas aparecem aqui.", self.open_url_item)
        self.recent_list = self.home_card(cards, "◷", "Recentes", "As páginas desta aba aparecem aqui.", self.open_url_item)
        col.addLayout(cards); wrap.addWidget(center, 0, Qt.AlignmentFlag.AlignHCenter); wrap.addStretch(2)
        return scroll

    def home_card(self, parent, icon, title, subtitle, callback):
        card = QFrame(objectName="card"); box = QVBoxLayout(card); box.setContentsMargins(17, 15, 17, 15); box.setSpacing(5)
        box.addWidget(label(icon + "  " + title, "sectionTitle")); box.addWidget(label(subtitle, "sectionNote"))
        listing = QListWidget(); listing.setMaximumHeight(100); listing.setMinimumHeight(62); listing.itemClicked.connect(callback); box.addWidget(listing)
        parent.addWidget(card); return listing

    def build_page(self):
        page = QWidget(objectName="page"); v = QVBoxLayout(page); v.setContentsMargins(34, 20, 34, 14)
        info = QHBoxLayout(); info.addWidget(label("PÁGINA ATUAL", "eyebrow")); info.addStretch(); info.addWidget(label("RENDERIZAÇÃO TEXTUAL", "sectionNote")); v.addLayout(info)
        self.page_url = label("", "pageUrl"); self.page_url.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse); v.addWidget(self.page_url)
        self.content = QPlainTextEdit(); self.content.setReadOnly(True); self.content.setAccessibleName("Conteúdo da página"); v.addWidget(self.content, 1)
        self.links_label = label("Links da página", "pageTitle"); v.addWidget(self.links_label)
        self.links = QListWidget(); self.links.setMaximumHeight(150); self.links.itemClicked.connect(lambda item: self.run_action("link", str(self.links.row(item)))); v.addWidget(self.links)
        return page

    def navigate(self, value):
        url = destination(value)
        if url: self.run_action("navigate", url)

    def open_url_item(self, item):
        self.navigate(item.text())

    def run_action(self, action, value=""):
        if self.worker is not None: return
        self.top.setEnabled(False); self.stack.setEnabled(False)
        self.progress.setRange(0, 0); self.progress.show(); self.status.setText("Carregando no motor Snovi…")
        self.worker = Worker(self.engine, action, value)
        self.worker.result.connect(self.on_result)
        self.worker.finished.connect(self.on_finished)
        self.worker.start()

    def on_result(self, data):
        if "fatal" in data:
            QMessageBox.critical(self, "Snovi", data["fatal"]); return
        self.data = data
        self.refresh()
        if data["error"]: QMessageBox.warning(self, "Não foi possível navegar", data["error"])

    def on_finished(self):
        self.top.setEnabled(True); self.stack.setEnabled(True); self.progress.hide()
        self.worker.deleteLater(); self.worker = None
        self.update_controls()

    def refresh(self):
        d = self.data
        self.tabs.blockSignals(True)
        while self.tabs.count(): self.tabs.removeTab(0)
        for url in d["tabs"]:
            title = urlsplit(url).hostname if url else "Nova aba"
            self.tabs.addTab(title or "Nova aba")
        self.tabs.setCurrentIndex(d["currentTab"]); self.tabs.blockSignals(False)
        self.address.setText(d["url"])
        self.star.setText("★" if d["url"] and d["url"] in d["favorites"] else "☆")
        self.favorite_list.clear(); self.favorite_list.addItems(d["favorites"])
        self.recent_list.clear(); self.recent_list.addItems(list(reversed(d["history"][-5:])))
        self.stack.setCurrentIndex(1 if d["url"] else 0)
        self.page_url.setText(d["url"])
        self.content.setPlainText(d["text"])
        font = QFont("DejaVu Sans", max(7, round(12 * d["zoom"] / 100))); self.content.setFont(font)
        self.links.clear(); self.links.addItems([f"{i+1}   {url}" for i, url in enumerate(d["links"])])
        self.links.setVisible(bool(d["links"])); self.links_label.setVisible(bool(d["links"]))
        self.status.setText(f"MOTOR SNOVI EXPERIMENTAL   ·   Modo texto   ·   Zoom {d['zoom']}%   ·   {d['blocked']} requisições bloqueadas")
        self.update_controls()

    def update_controls(self):
        d = self.data
        self.back.setEnabled(d["canBack"]); self.forward.setEnabled(d["canForward"])
        self.reload.setEnabled(bool(d["url"])); self.star.setEnabled(bool(d["url"]))

    def open_menu(self):
        d = self.data; menu = QMenu(self)
        def add(name, callback, enabled=True):
            action = menu.addAction(name); action.setEnabled(enabled); action.triggered.connect(callback)
        add("＋   Nova aba", lambda: self.run_action("new_tab"))
        menu.addSeparator()
        add("‹    Voltar", lambda: self.run_action("back"), d["canBack"])
        add("›    Avançar", lambda: self.run_action("forward"), d["canForward"])
        add("⟳   Atualizar página", lambda: self.run_action("reload"), bool(d["url"]))
        add("⌘   Ver código-fonte", self.show_source, bool(d["url"]))
        add("↗   Copiar endereço da página", self.copy_url, bool(d["url"]))
        menu.addSeparator()
        zoom = menu.addMenu(f"Zoom da página   {d['zoom']}%")
        for name, z in [("Diminuir  −", d["zoom"]-10), ("Restaurar  100%", 100), ("Aumentar  +", d["zoom"]+10)]:
            zoom.addAction(name).triggered.connect(lambda _=False, z=z: self.run_action("zoom", str(z)))
        add("◷   Histórico desta aba", lambda: self.show_urls("Histórico", d["history"]))
        add("☆   Favoritos", lambda: self.show_urls("Favoritos", d["favorites"]))
        menu.addSeparator()
        add("◈   Filtro de rede: " + ("ativado" if d["adblockEnabled"] else "desativado"), lambda: self.run_action("adblock"))
        add(f"     Carregar lista de domínios ({d['filterCount']})", self.load_filter)
        add("ⓘ   Sobre o Snovi", self.about)
        menu.exec(self.menu_button.mapToGlobal(self.menu_button.rect().bottomLeft()))

    def show_source(self):
        dialog = QDialog(self); dialog.setWindowTitle("Código-fonte · Snovi"); dialog.resize(850, 600)
        layout = QVBoxLayout(dialog); layout.addWidget(label(self.data["url"]))
        source = QPlainTextEdit(); source.setReadOnly(True); source.setPlainText(self.data["source"]); source.setFont(QFont("DejaVu Sans Mono", 10)); layout.addWidget(source)
        dialog.exec()

    def copy_url(self):
        QApplication.clipboard().setText(self.data["url"]); self.status.setText("Endereço copiado para a área de transferência.")

    def show_urls(self, title, urls):
        dialog = QDialog(self); dialog.setWindowTitle(title + " · Snovi"); dialog.resize(550, 300)
        layout = QVBoxLayout(dialog); layout.addWidget(label(title, "sectionTitle")); listing = QListWidget(); listing.addItems(list(reversed(urls))); layout.addWidget(listing)
        listing.itemDoubleClicked.connect(lambda item: (dialog.accept(), self.navigate(item.text())))
        layout.addWidget(label("Clique duas vezes para abrir. Dados mantidos apenas nesta sessão.", "sectionNote")); dialog.exec()

    def load_filter(self):
        path, _ = QFileDialog.getOpenFileName(self, "Carregar lista de domínios", "", "Arquivos de texto (*.txt);;Todos os arquivos (*)")
        if path: self.run_action("filter", path)

    def about(self):
        QMessageBox.information(self, "Sobre o Snovi", "Snovi Browser · versão experimental\n\nMotor próprio em C++; interface Qt Widgets. Sem WebView.\n\nA renderização atual é textual. CSS, JavaScript, imagens, segurança de navegador completo e aplicativos móveis ainda não estão implementados.")

    def closeEvent(self, event):
        if self.worker is not None:
            QMessageBox.information(self, "Snovi", "Aguarde o término da requisição antes de fechar."); event.ignore(); return
        self.engine.close(); event.accept()


def main():
    app = QApplication(sys.argv); app.setStyle("Fusion"); app.setStyleSheet(STYLES)
    try:
        engine = Engine()
    except (OSError, RuntimeError) as exc:
        QMessageBox.critical(None, "Snovi", "Não foi possível carregar o motor. Compile o projeto primeiro.\n\n" + str(exc)); return 1
    window = BrowserWindow(engine); window.show()
    if len(sys.argv)>1 and sys.argv[1] == "--self-test":
        # Packaging smoke test: instantiate the actual window and native bridge, then exit.
        QTimer.singleShot(250, window.close)
    elif len(sys.argv)>1:
        window.navigate(sys.argv[1])
    return app.exec()

if __name__ == "__main__":
    sys.exit(main())
