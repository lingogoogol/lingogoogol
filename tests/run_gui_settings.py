#!/usr/bin/env python3
"""Operate the production Windows settings/clear dialogs using only a local fake Drive."""
import json
import hashlib
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
        if path.startswith('/fixture/'):
            self.server.phase = path.rsplit('/', 1)[-1]
            if self.server.phase != 'cancel': self.server.release.set()
            return self.reply(200, {})
        if path == '/drive/v3/files':
            return self.reply(200, {'files': [{'id': 'root'}, {'id': 'outside-app-root'}]})
        return self.reply(200, {'name': '{"next":"null"}', 'description': '[]'})

    def delete_result(self, path):
        phase = self.server.phase
        if phase == 'cancel':
            # Keep a DELETE in flight until the user cancels its curl handle.
            # A separate fixture request releases this server handler afterward.
            (self.server.test / 'delete-pending').touch()
            self.server.release.wait(20)
            return 503, {}
        if phase == 'failure':
            return 503, {}
        with self.server.lock:
            self.server.deleted.add(urlparse(path).path.rsplit('/', 1)[-1])
        return 204, b''

    def do_DELETE(self):
        self.reply(*self.delete_result(self.path))

    def do_POST(self):
        body = self.body()
        if not body:
            return
        if urlparse(self.path).path == '/drive/v3/files':
            return self.reply(200, {'id': 'new-root'})
        requests = re.findall(rb'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--)', body, re.S)
        assert 0 < len(requests) <= 100
        parts = []
        for identifier, request in requests:
            method, path, *_ = request.split(b'\r\n', 1)[0].decode().split(' ')
            if method == 'DELETE':
                status, value = self.delete_result(path)
            else:
                with self.server.lock:
                    self.server.serial += 1
                    status, value = 200, {'id': f'new-blob-{self.server.serial}'}
            data = value if isinstance(value, bytes) else json.dumps(value).encode()
            parts.append(b'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-' +
                         identifier + f'\r\n\r\nHTTP/1.1 {status} OK\r\n\r\n'.encode() + data + b'\r\n')
        self.reply(200, b''.join(parts) + b'--fixture--\r\n', 'multipart/mixed; boundary=fixture')


# Coordinates are client coordinates at the production 1536 x 1024 initial size.
# No real file picker, credentials or Drive endpoint is involved.
interaction = r'''
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import os
from urllib.request import urlopen

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
    run('keydown', 'F3')
    time.sleep(.3)
    capture('f3-desktop')
    run('keyup', 'F3')
    time.sleep(.3)
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
    def snapshot():
        path = Path('clear-ui.json')
        return json.loads(path.read_text()) if path.exists() else {}
    def wait_ui(predicate, message):
        wait_for(lambda: predicate(snapshot()), message)
        return snapshot()
    def action(name):
        wait_ui(lambda s: s.get('settled', False), 'Progress layout did not settle.')
        value = snapshot()[name]
        assert value['shown'], name + ' is hidden.'
        click(value['x'] + value['w'] // 2, value['y'] + value['h'] // 2)
    def mode(value):
        with urlopen('http://127.0.0.1:' + os.environ['STORAGE_GPU_TEST_PORT'] + '/fixture/' + value) as response:
            response.read()
    def start_clear():
        click(1480, 24)
        click(500, 540)
        subprocess.run(['wine64', sys.argv[2], '清除'], check=True)
        run('key', 'ctrl+v')
        click(1056, 632)
    def acknowledge_cancel():
        value = wait_ui(lambda s: s.get('text') == '清除：已取消', 'Cancelled clear did not finish draining.')
        assert value['confirm']['shown'] and not value['retry']['shown'] and not value['cancel']['shown']
        assert value['confirm']['x'] == value['retry']['x']
        assert list(Path('.').glob('clear.*')), 'Cancellation removed the resumable inventory.'
        action('confirm')
        wait_ui(lambda s: s.get('rows') == 0, 'Confirm did not remove the cancelled row.')
    mode('cancel')
    click(1480, 24)  # Clear
    click(1056, 632)  # Empty confirmation must be rejected.
    assert (Path('root') / hashlib.sha256(b'mock@example.invalid').hexdigest()).read_text() == 'root'
    click(500, 540)
    subprocess.run(['wine64', sys.argv[2], '清除'], check=True)
    run('key', 'ctrl+v')
    time.sleep(.3)
    capture('clear-dialog')
    click(936, 632)  # Typed confirmation followed by Cancel must still do nothing.
    assert (Path('root') / hashlib.sha256(b'mock@example.invalid').hexdigest()).read_text() == 'root'
    click(1480, 24)
    click(500, 540)
    subprocess.run(['wine64', sys.argv[2], '清除'], check=True)
    run('key', 'ctrl+v')
    time.sleep(.3)
    click(1056, 632)
    value = wait_ui(lambda s: s.get('rows') == 1 and s['cancel']['shown'], 'Running clear controls did not appear.')
    assert value['cancel']['x'] == value['retry']['x'], 'Sole Cancel reserves a hidden Retry slot.'
    assert value['cancel']['y'] >= value['text_y'] + value['text_h'], 'Actions overlap clear text.'
    wait_for(lambda: bool(list(Path('.').glob('clear.*'))), 'Clear inventory was not checkpointed.')
    wait_for(lambda: Path('delete-pending').exists(), 'Clear did not submit its DELETE request.')
    capture('clear-running')
    action('cancel')
    acknowledge_cancel()
    mode('failure')
    start_clear()  # A fresh clear must reset the previous cancellation flag.
    value = wait_ui(lambda s: s.get('text', '').startswith('清除失敗：'), 'Network error controls did not appear.')
    assert value['retry']['shown'] and value['cancel']['shown'] and value['separator']['shown']
    assert value['cancel']['x'] >= value['separator']['x'] + value['separator']['w'], 'Cancel overlaps the separator.'
    assert value['retry']['y'] == value['cancel']['y'], 'Retry and Cancel are on different lines.'
    capture('clear-network-failure')
    action('cancel')
    acknowledge_cancel()
    start_clear()
    wait_ui(lambda s: s.get('text', '').startswith('清除失敗：'), 'Second clear did not reach the network error.')
    mode('success')
    action('retry')
    wait_for(lambda: (Path('root') / hashlib.sha256(b'mock@example.invalid').hexdigest()).read_text() == 'new-root', 'Retried clear did not finish.')
    value = wait_ui(lambda s: s.get('text') == '清除完成', 'Clear completion was not displayed.')
    assert value['confirm']['shown'] and not value['retry']['shown'] and not value['cancel']['shown']
    assert value['confirm']['x'] == value['retry']['x']
    action('confirm')
    wait_ui(lambda s: s.get('rows') == 0, 'Confirm did not remove the completed row.')
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
    entry = next(item for item in entries if Path(item['file']) == source / 'storage/storage.cpp')
    observed = (source / 'storage/storage.cpp').read_text(encoding='utf-8-sig')
    # Expose the inherited visibility query only in this test executable.
    observed = observed.replace('progress_button_t', 'observed_t<progress_button_t>')
    observed = observed.replace('lgo::std_liney_t', 'observed_t<lgo::std_liney_t>')
    observed = observed.replace('std::atomic_bool close_app', '''template<class Base> struct observed_t : Base
    {
        using Base::Base;
        auto is_shown() const -> bool { return this->status() == this->shown; }
    };
    std::atomic_bool close_app''')
    observation = r'''        if (clear_text)
        {
            const auto button = [](const auto& object)
            {
                const auto pos{object->get_pos()}; const auto size{object->get_size()};
                return nlohmann::json{{"shown", object->is_shown()},
                    {"x", pos.x}, {"y", pos.y}, {"w", size.x}, {"h", size.y}};
            };
            const auto value = nlohmann::json{{"rows", clear_rows.size()}, {"settled", !progress_column->children_moving()}, {"text", clear_text->get_text()},
                {"text_y", clear_text->get_pos().y}, {"text_h", clear_text->get_size().y},
                {"retry", button(clear_retry)}, {"cancel", button(clear_cancel)},
                {"confirm", button(clear_confirm)}, {"separator", button(clear_separator)}}.dump();
            static std::string previous;
            if (value != previous) { storage_atomic_write("clear-ui.json", value); previous = value; }
        }
'''
    observed = observed.replace('        lgo::text_line_t::pending_load();', observation + '        lgo::text_line_t::pending_load();')
    (test / 'observed.cpp').write_text(observed)
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(test / 'entry.obj')
    command[command.index('-c') + 1] = str(test / 'observed.cpp')
    subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = shlex.split(link.split(' && ')[1])
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'entry.obj')
    command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(test / 'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in (runtime / 'bin').glob('*.dll'):
        shutil.copy2(library, test / library.name)
    for resource in ('font', 'shader'):
        shutil.copytree(runtime / resource, test / resource)
    shutil.copy2(test / 'font/NotoSansTC-VariableFont_wght.ttf', test / 'font/alternate.ttf')
    (test / 'root').mkdir()
    (test / 'blob_queue').mkdir()
    (test / 'key.json').write_text(json.dumps({'client_email': 'mock@example.invalid', 'private_key': 'unused-fixture-key'}))
    (test / 'root' / hashlib.sha256(b'mock@example.invalid').hexdigest()).write_text('root')
    (test / 'blob_queue' / hashlib.sha256(b'mock@example.invalid').hexdigest()).write_text(json.dumps(['gui-placeholder'] * 21000))
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    server.lock = threading.Lock()
    server.deleted = set()
    server.serial = 0
    server.test = test
    server.phase = 'cancel'
    server.release = threading.Event()
    threading.Thread(target=server.serve_forever, daemon=True).start()
    process = None
    try:
        process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'python3', '-c', interaction, str(test / 'test.exe'), str(test / 'clipboard.exe')],
                                   cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
        stdout, stderr = process.communicate(timeout=100)
        if process.returncode:
            raise RuntimeError(stdout + stderr)
        assert server.deleted == {'root', 'outside-app-root'}, 'Clear scope differs from the confirmed whole account.'
        logs = ''.join(path.read_text(errors='replace') for path in (test / 'changed-logfile').glob('*.txt'))
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
        assert not list(test.glob('clear.*')), 'Completed clear marker was retained.'
        print('PASS: production GUI Cancel/OK, UTF-8 paste, Backspace, Ctrl+A, live settings/font/log reload, typed whole-account clear, running/error cancellation, action geometry, retry, terminal acknowledgement and clean close')
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
        server.shutdown()
        server.server_close()
