import http.server
import subprocess
import ssl
import tempfile
import shutil
import os
from pathlib import Path
import sys
import threading

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/page')
            self.end_headers()
        elif self.path == '/chunked':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Transfer-Encoding', 'chunked')
            self.end_headers()
            self.wfile.write(b'5\r\nHello\r\n0\r\n\r\n')
        else:
            body = b'<html><body><h1>Local test</h1><a href="/chunked">Chunk</a></body></html>'
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
    def log_message(self, *args): pass

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    url = f'http://127.0.0.1:{server.server_port}/redirect'
    run = subprocess.run([sys.argv[1], url], input='fonte\nlink 1\nvoltar\naba nova\nabas\nsair\n', text=True, encoding='utf-8', capture_output=True, timeout=20, check=True)
    assert '/page' in run.stdout and 'Local test' in run.stdout and '<h1>Local test</h1>' in run.stdout
    assert 'Hello' in run.stdout and 'Início' in run.stdout, run.stdout
    assert not run.stderr, run.stderr
finally:
    server.shutdown()
    server.server_close()

# A self-signed certificate must never be silently accepted.
if shutil.which('openssl'):
    with tempfile.TemporaryDirectory() as tmp:
        key, cert = Path(tmp) / 'key.pem', Path(tmp) / 'cert.pem'
        subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes',
                        '-keyout', str(key), '-out', str(cert), '-days', '1',
                        '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost'], check=True, capture_output=True)
        tls_server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert, key)
        tls_server.socket = context.wrap_socket(tls_server.socket, server_side=True)
        threading.Thread(target=tls_server.serve_forever, daemon=True).start()
        try:
            url = f'https://127.0.0.1:{tls_server.server_port}/'
            run = subprocess.run([sys.argv[1], url], input='sair\n', text=True, encoding='utf-8', capture_output=True, timeout=20, check=True)
            assert 'certificado inválido' in run.stderr, run.stderr
            assert 'Local test' not in run.stdout
            # Trust the test certificate, then exercise both hostname success and mismatch.
            env = dict(os.environ, SSL_CERT_FILE=str(cert))
            good = subprocess.run([sys.argv[1], f'https://localhost:{tls_server.server_port}/'], input='sair\n', text=True, encoding='utf-8', capture_output=True, timeout=20, check=True, env=env)
            assert 'Local test' in good.stdout and not good.stderr, (good.stdout, good.stderr)
            mismatch = subprocess.run([sys.argv[1], url], input='sair\n', text=True, encoding='utf-8', capture_output=True, timeout=20, check=True, env=env)
            assert 'certificado inválido' in mismatch.stderr and 'Local test' not in mismatch.stdout, mismatch.stderr
        finally:
            tls_server.shutdown()
            tls_server.server_close()
