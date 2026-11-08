"""Run against a running emulator after installing the debug APK."""
import http.server
import re
import subprocess
import threading
import time
import xml.etree.ElementTree as ET


def adb(*args):
    return subprocess.check_output(['adb', *args], text=True, timeout=30)


def dump():
    adb('shell', 'uiautomator', 'dump', '/sdcard/snovi.xml')
    return adb('shell', 'cat', '/sdcard/snovi.xml')


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = b'<html><body><h1>Snovi teste local</h1><a href="/next">Link local</a></body></html>'
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *args):
        pass


server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
port = str(server.server_port)
try:
    adb('reverse', 'tcp:' + port, 'tcp:' + port)
    adb('shell', 'am', 'start', '-W', '-n', 'com.snovi.browser.experimental/com.snovi.browser.MainActivity')
    assert adb('shell', 'pidof', 'com.snovi.browser.experimental').strip()
    initial = dump()
    assert 'BROWSE BEYOND' in initial, initial
    root = ET.fromstring(initial)
    field = next(n for n in root.iter('node') if n.get('class') == 'android.widget.EditText')
    x1, y1, x2, y2 = map(int, re.findall(r'\d+', field.get('bounds')))
    adb('shell', 'input', 'tap', str((x1+x2)//2), str((y1+y2)//2))
    adb('shell', 'input', 'text', f'http://127.0.0.1:{port}/')
    adb('shell', 'input', 'keyevent', '66')
    deadline = time.monotonic() + 30
    while True:
        screen = dump()
        if 'Snovi teste local' in screen:
            break
        if time.monotonic() >= deadline:
            raise AssertionError('Page not rendered: ' + screen)
        time.sleep(1)
    assert 'Link local' in screen or '/next' in screen, screen
    print('Android: application opened and rendered a local HTTP page using the native engine')
finally:
    server.shutdown()
    server.server_close()
    subprocess.run(['adb', 'reverse', '--remove', 'tcp:' + port], check=False)
