#!/usr/bin/env python3
"""Render the phone-width (compact) layout against a local Drive and save screenshots.

Usage: run_compact_layout.py [output directory]  (default: a temporary directory that is printed)
"""
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'
title = 'Compact layout regression'

def blob(name, content):
    return {'name': json.dumps(name), 'description': json.dumps(content)}

def entry(id, kind, name, size, time_value):
    return {'id': id, 'account': 'fixture', 'type': kind, 'name': name}

def item(id, kind, name, size):
    metadata = {'name': name, 'parent': 'root', 'parent_account': 'fixture', 'next': 'null', 'next_account': 'null',
                'account': 'fixture', 'size': size, 'version': 2, 'version_min': 2 if kind == 'file' else 1,
                'time': 1793000000}
    if kind == 'folder':
        metadata['type'] = 'folder'
    return blob(metadata, [])

files = [('trip', 'folder', '旅行', 1288490188), ('photo', 'file', 'IMG_20261103_091522.jpg', 5033164),
         ('tax', 'file', '報稅資料_2025.pdf', 831488), ('video', 'file', 'VID_20261110_184402.mp4', 2480343613),
         ('notes', 'file', '一個名稱很長的會議記錄與附件說明文件_最終版_第三次修訂.docx', 48211)]

class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def log_message(self, *args): pass
    def do_GET(self):
        id = urlparse(self.path).path.rsplit('/', 1)[-1]
        found = self.server.blobs.get(id)
        body = json.dumps(found if found else {}).encode()
        # The download task waits on this response, keeping its progress row visible.
        if id == 'photo': time.sleep(30)
        self.send_response(200 if found else 404)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

def build_test(test):
    dialog = (source / 'storage/dialog/.cpp').read_text()
    start = dialog.index('dialog_t::dialog_t(bool folder)')
    end = dialog.index('auto dialog_t::ok()', start)
    (test / 'dialog.cpp').write_text(dialog[:start] + 'dialog_t::dialog_t(bool) { m_result.push_back("output"); m_ok=true; }\n' + dialog[end:])
    commands = json.loads((build / 'compile_commands.json').read_text())
    for original, replacement, output in [('storage/storage.cpp', source / 'tests/compact_layout.cpp', 'test.obj'),
                                          ('storage/google/easy/.cpp', source / 'tests/mock_google_easy.cpp', 'mock.obj'),
                                          ('storage/dialog/.cpp', test / 'dialog.cpp', 'dialog.obj')]:
        command = shlex.split(next(e['command'] for e in commands if Path(e['file']) == source / original))
        command[command.index('-o') + 1] = str(test / output)
        command[command.index('-c') + 1] = str(replacement)
        subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = [x for x in shlex.split(link.split(' && ')[1]) if x not in ('-mwindows', '-municode') and not x.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'test.obj')
    command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(test / 'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    command[command.index('storage/dialog/libstorage.dialog.a')] = str(test / 'dialog.obj')
    command.append('storage/dialog/CMakeFiles/storage.dialog.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)
    for path in (runtime / 'bin').glob('*.dll'):
        shutil.copy2(path, test / path.name)
    for name in ['font', 'shader']:
        shutil.copytree(runtime / name, test / name)
    (test / 'root').mkdir()
    (test / 'queue').write_text(json.dumps([f'reserve-{i}' for i in range(20000)]))

# Runs inside xvfb-run: answers each capture request with a screenshot of the window.
driver = r'''
import subprocess, sys, time
from pathlib import Path
app = subprocess.Popen(['wine64', sys.argv[1]])
output = Path(sys.argv[2])
captured = []
deadline = time.monotonic() + 240
while app.poll() is None and time.monotonic() < deadline:
    request = Path('capture-request')
    if request.exists():
        name = request.read_text()
        window = subprocess.check_output(['xdotool', 'search', '--onlyvisible', '--name', sys.argv[3]], text=True).split()[0]
        geometry = dict(line.strip().split('=', 1) for line in subprocess.check_output(
            ['xdotool', 'getwindowgeometry', '--shell', window], text=True).splitlines() if '=' in line)
        assert int(geometry['WIDTH']) < 600, f'window widened to {geometry["WIDTH"]}'
        subprocess.run(['import', '-window', window, str(output / f'{name}.png')], check=True)
        captured.append(name)
        request.unlink()
    time.sleep(.05)
if app.poll() is None:
    app.kill()
    raise SystemExit('timed out; captured ' + ', '.join(captured))
sys.exit(app.returncode)
'''

output = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(tempfile.mkdtemp(prefix='storage-compact-shots-'))
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='storage-compact-') as directory:
    test = Path(directory)
    build_test(test)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    server.blobs = {'root': blob({'name': 'root', 'parent': 'null', 'parent_account': 'null', 'next': 'null', 'next_account': 'null',
                                  'version': 2, 'version_min': 1, 'type': 'folder', 'size': 0},
                                 [entry(id, kind, name, size, 0) for id, kind, name, size in files])}
    for id, kind, name, size in files:
        server.blobs[id] = item(id, kind, name, size)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    result = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 412x860x24', 'python3', '-c', driver,
                             str(test / 'test.exe'), str(output), title], cwd=test,
                            env={**__import__('os').environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                            capture_output=True, text=True, timeout=300)
    print(result.stdout, end='')
    shots = sorted(path.name for path in output.glob('*.png'))
    print('Screenshots in', output, shots)
    if result.returncode or len(shots) != 4:
        print(result.stderr[-3000:])
        shutil.copytree(test, '/tmp/storage-compact-failed', dirs_exist_ok=True)
        print('Test directory copied to /tmp/storage-compact-failed')
        sys.exit(1)
