#!/usr/bin/env python3
"""Operate the production Windows settings/clear dialogs using only a local fake Drive."""
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *unused):
        pass

    def handle(self):
        try:
            super().handle()
        except ConnectionResetError:
            pass

    def body(self):
        if self.headers.get('Transfer-Encoding') == 'chunked':
            result = b''
            while True:
                line = self.rfile.readline().strip().split(b';')[0]
                if not line:
                    return result
                count = int(line, 16)
                if not count:
                    self.rfile.readline()
                    return result
                result += self.rfile.read(count)
                self.rfile.read(2)
        return self.rfile.read(int(self.headers.get('Content-Length', 0)))

    def reply(self, status, value, content_type='application/json'):
        data = value if isinstance(value, bytes) else json.dumps(value).encode()
        self.send_response(status)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Content-Type', content_type)
        self.end_headers()
        try:
            self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_GET(self):
        path = urlparse(self.path).path
        if path == '/drive/v3/files':
            return self.reply(200, {'files': [{'id': 'root'}, {'id': 'outside-app-root'}]})
        return self.reply(200, {'name': '{"next":"null"}', 'description': '[]'})

    def do_DELETE(self):
        with self.server.lock:
            self.server.deleted.add(urlparse(self.path).path.rsplit('/', 1)[-1])
        self.reply(204, b'')

    def do_POST(self):
        body = self.body()
        if not body:
            return
        if urlparse(self.path).path == '/drive/v3/files':
            return self.reply(200, {'id': 'new-root'})
        identifiers = re.findall(rb'Content-ID: (\d+)\r\n', body)
        parts = []
        with self.server.lock:
            for identifier in identifiers:
                self.server.serial += 1
                parts.append(b'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-' +
                             identifier + b'\r\n\r\nHTTP/1.1 200 OK\r\n\r\n' +
                             json.dumps({'id': f'new-blob-{self.server.serial}'}).encode() + b'\r\n')
        self.reply(200, b''.join(parts) + b'--fixture--\r\n', 'multipart/mixed; boundary=fixture')


# Coordinates are client coordinates at the production 1536 x 1024 initial size.
# No real file picker, credentials or Drive endpoint is involved.
interaction = r'''
import json
from pathlib import Path
import subprocess
import sys
import time

app = subprocess.Popen(['wine64', sys.argv[1]])
def run(*args):
    return subprocess.run(['xdotool', *map(str, args)], check=True, capture_output=True, text=True)
def wait_for(predicate, message, timeout=15):
    deadline = time.monotonic() + timeout
    while not predicate():
        if app.poll() is not None or time.monotonic() > deadline:
            raise RuntimeError(message)
        time.sleep(.05)
try:
    window = None
    def find_window():
        global window
        result = subprocess.run(['xdotool', 'search', '--onlyvisible', '--name', 'wWinMain.engine'], capture_output=True, text=True)
        if result.returncode == 0:
            window = result.stdout.splitlines()[0]
            return True
        return False
    wait_for(find_window, 'Window did not appear.', 30)
    run('windowfocus', '--sync', window)
    time.sleep(1)
    def click(x, y):
        run('mousemove', '--window', window, x, y, 'click', '1')
        time.sleep(.2)
    def capture(name):
        try:
            from PIL import ImageGrab
            ImageGrab.grab().save('/tmp/storage-' + name + '.png')
        except ImportError:
            pass
    click(1064, 24)  # Settings
    capture('settings-dialog')
    click(936, 740)  # Cancel
    assert not Path('settings.json').exists(), 'Cancel saved settings.'
    click(1064, 24)
    click(500, 372)
    run('key', 'ctrl+a')
    run('type', 'changed-logfile')
    click(500, 440)
    run('key', 'ctrl+a')
    run('type', 'font/alternate.ttf')
    click(500, 644)
    run('key', 'BackSpace', 'BackSpace')
    run('type', '2')
    click(1056, 740)
    wait_for(lambda: Path('settings.json').exists(), 'Settings were not applied.')
    value = json.loads(Path('settings.json').read_text())
    assert value['logfile_path'] == 'changed-logfile' and value['network_retries'] == 2
    capture('settings-live')
    click(1480, 24)  # Clear
    click(1056, 632)  # Empty confirmation must be rejected.
    assert Path('root').read_text() == 'root'
    click(500, 540)
    subprocess.run(['wine64', sys.argv[2], '清除'], check=True)
    run('key', 'ctrl+v')
    time.sleep(.3)
    capture('clear-dialog')
    click(936, 632)  # Typed confirmation followed by Cancel must still do nothing.
    assert Path('root').read_text() == 'root'
    click(1480, 24)
    click(500, 540)
    subprocess.run(['wine64', sys.argv[2], '清除'], check=True)
    run('key', 'ctrl+v')
    time.sleep(.3)
    click(1056, 632)
    wait_for(lambda: Path('root').read_text() == 'new-root', 'Confirmed clear did not finish.')
    run('key', 'alt+F4')
    app.wait(timeout=10)
    assert app.returncode == 0, 'Application did not close normally.'
finally:
    if app.poll() is None:
        app.kill()
        app.wait()
'''

with tempfile.TemporaryDirectory(prefix='storage-gui-settings-') as directory:
    test = Path(directory)
    # Set CF_UNICODETEXT inside Wine. X11 clipboard bridges can return an empty
    # selection despite xclip owning UTF8_STRING, which does not test the app.
    clipboard = test / 'clipboard.cpp'
    clipboard.write_text(r'''#include <windows.h>
#include <cwchar>
#include <cstring>
int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || !OpenClipboard(nullptr)) return 1;
    const auto size = (std::wcslen(argv[1]) + 1) * sizeof(wchar_t);
    auto memory = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!memory) { CloseClipboard(); return 2; }
    auto data = GlobalLock(memory);
    if (!data) { GlobalFree(memory); CloseClipboard(); return 3; }
    std::memcpy(data, argv[1], size);
    GlobalUnlock(memory);
    EmptyClipboard();
    const auto result = SetClipboardData(CF_UNICODETEXT, memory);
    CloseClipboard();
    if (!result) GlobalFree(memory);
    return result ? 0 : 4;
}
''')
    subprocess.run(['x86_64-w64-mingw32-clang++', '-municode', str(clipboard), '-o', str(test / 'clipboard.exe')], check=True)
    entries = json.loads((build / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']) == source / 'storage/google/easy/.cpp')
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(test / 'mock.obj')
    command[command.index('-c') + 1] = str(source / 'tests/mock_google_easy.cpp')
    subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = shlex.split(link.split(' && ')[1])
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(test / 'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in runtime.glob('*.dll'):
        shutil.copy2(library, test / library.name)
    for resource in ('font', 'shader'):
        shutil.copytree(runtime / resource, test / resource)
    shutil.copy2(test / 'font/NotoSansTC-VariableFont_wght.ttf', test / 'font/alternate.ttf')
    (test / 'key.json').write_text(json.dumps({'client_email': 'mock@example.invalid', 'private_key': 'unused-fixture-key'}))
    (test / 'root').write_text('root')
    (test / 'blob_queue').write_text(json.dumps(['gui-placeholder'] * 21000))
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    server.lock = threading.Lock()
    server.deleted = set()
    server.serial = 0
    threading.Thread(target=server.serve_forever, daemon=True).start()
    process = None
    try:
        process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'python3', '-c', interaction, str(test / 'test.exe'), str(test / 'clipboard.exe')],
                                   cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
        stdout, stderr = process.communicate(timeout=60)
        if process.returncode:
            raise RuntimeError(stdout + stderr)
        assert server.deleted == {'root', 'outside-app-root'}, 'Clear scope differs from the confirmed whole account.'
        logs = ''.join(path.read_text(errors='replace') for path in (test / 'changed-logfile').glob('*.txt'))
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
        assert not list(test.glob('clear.*')), 'Completed clear marker was retained.'
        print('PASS: production GUI Cancel/OK, UTF-8 paste, Backspace, Ctrl+A, live settings/font/log reload, typed whole-account clear, invalid/declined confirmation and clean close')
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
        server.shutdown()
        server.server_close()
