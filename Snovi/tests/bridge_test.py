import http.server
import sys
import tempfile
import threading
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'UI' / 'Desktop'))
from engine import Engine

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = (b'<h1>Pagina real</h1><a href="/next">Avancar</a>' if self.path != '/next' else b'<p>Segunda pagina</p>')
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *args): pass

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
engine = Engine(sys.argv[1])
try:
    base = f'http://127.0.0.1:{server.server_port}'
    d = engine.execute('navigate', base + '/')
    assert d['url'] == base + '/' and 'Pagina real' in d['text'] and '<h1>' in d['source'] and d['links'] == ['/next'], d
    d = engine.execute('favorite'); assert d['favorites'] == [base + '/']
    d = engine.execute('link', '0'); assert d['url'] == base + '/next' and d['canBack']
    d = engine.execute('back'); assert d['canForward'] and d['url'] == base + '/'
    d = engine.execute('zoom', '140'); assert d['zoom'] == 140
    d = engine.execute('new_tab'); assert len(d['tabs']) == 2 and not d['url']
    d = engine.execute('select_tab', '0'); assert d['url'] == base + '/'
    d = engine.execute('close_tab', '0'); assert len(d['tabs']) == 1 and not d['url']
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / 'filter.txt'
        path.write_text('127.0.0.1\n')
        assert engine.execute('filter', str(path))['filterCount'] == 1
        d = engine.execute('navigate', base + '/'); assert d['error'] and d['blocked'] == 1 and not d['url']
        engine.execute('adblock')
        assert engine.execute('navigate', base + '/')['url'] == base + '/'
finally:
    engine.close(); server.shutdown(); server.server_close()
