"""Optional GUI smoke test: QT_QPA_PLATFORM=offscreen python3 desktop_test.py /path/to/libsnovi_bridge.so"""
import http.server
import sys
import threading
import time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'UI' / 'Desktop'))
from PySide6.QtWidgets import QApplication
from engine import Engine
from main import BrowserWindow, destination
from styles import STYLES

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = b'<h1>Site local</h1><a href="/next">Link</a>'
        self.send_response(200); self.send_header('Content-Type', 'text/html'); self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body)
    def log_message(self, *args): pass

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
app = QApplication([]); app.setStyleSheet(STYLES)
window = BrowserWindow(Engine(sys.argv[1])); window.show()
def wait():
    end = time.monotonic() + 10
    while window.worker is not None and time.monotonic() < end:
        app.processEvents(); time.sleep(.01)
    app.processEvents()
    assert window.worker is None, 'operation did not complete'
try:
    assert destination('github.com') == 'https://github.com'
    assert 'q=ola+mundo' in destination('ola mundo')
    base = f'http://127.0.0.1:{server.server_port}/'
    window.navigate(base); wait()
    assert window.data['url'] == base and 'Site local' in window.content.toPlainText()
    assert window.links.count() == 1
    window.run_action('link', '0'); wait(); assert window.data['url'] == base + 'next'
    window.back.click(); wait(); assert window.data['url'] == base
    window.star.click(); wait(); assert window.data['favorites'] == [base]
    window.run_action('zoom', '150'); wait(); assert window.data['zoom'] == 150
    window.run_action('new_tab'); wait(); assert window.stack.currentIndex() == 0 and window.tabs.count() == 2
    window.run_action('select_tab', '0'); wait(); assert window.stack.currentIndex() == 1
    window.copy_url(); assert app.clipboard().text() == base
finally:
    window.close(); app.processEvents(); server.shutdown(); server.server_close()
print('Desktop smoke test passed')
