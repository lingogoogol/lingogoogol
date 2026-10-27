#!/usr/bin/env python3
"""Test production Windows curl workers with local HTTP/TLS fixtures, without credentials.

Optional real interface-down/packet-drop trials: sudo -n unshare --net python3
tests/run_background_multi.py --offline. Only the isolated namespace is changed.
"""
import argparse
import contextlib
import fcntl
import http.server
import json
import os
from pathlib import Path
import pwd
import queue
import shlex
import shutil
import socket
import socketserver
import ssl
import struct
import subprocess
import tempfile
import threading
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--offline', action='store_true')
args = parser.parse_args()
if args.offline and os.readlink('/proc/self/ns/net') == os.readlink('/proc/1/ns/net'):
    raise SystemExit('Offline tests require an isolated network namespace: sudo -n unshare --net ...')

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'


def loopback(up):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as handle:
        query = fcntl.ioctl(handle, 0x8913, struct.pack('16sh', b'lo', 0))
        flags = struct.unpack('16sh', query[:18])[1]
        fcntl.ioctl(handle, 0x8914, struct.pack('16sh', b'lo', flags | 1 if up else flags & ~1))


def firewall(drop):
    if drop:
        subprocess.run(['nft', '-f', '-'], check=True, text=True, input=
            'add table inet storage_worker_test\n'
            'add chain inet storage_worker_test output { type filter hook output priority 0; policy accept; }\n'
            'add rule inet storage_worker_test output oifname "lo" drop\n')
    else:
        subprocess.run(['nft', 'delete', 'table', 'inet', 'storage_worker_test'],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


class Response(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def do_GET(self):
        if self.path == '/pause-download':
            self.send_response(200)
            self.send_header('Content-Length', '524288')
            self.end_headers()
            try:
                for unused in range(64):
                    self.wfile.write(b'd' * 8192)
                    self.wfile.flush()
                    time.sleep(.025)
            except (BrokenPipeError, ConnectionResetError):
                pass
            return
        data = b'background-ok'
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        if self.headers.get('Transfer-Encoding') == 'chunked':
            body = b''
            while True:
                line = self.rfile.readline().strip()
                if not line: return  # Intentional pause aborts an incomplete POST.
                length = int(line.split(b';')[0], 16)
                if not length:
                    self.rfile.readline()
                    break
                body += self.rfile.read(length)
                self.rfile.read(2)
        else:
            body = self.rfile.read(int(self.headers['Content-Length']))
        data = b'upload-ok' if body == b'u' * 524288 else b'bad-upload'
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *unused):
        pass


class Upload(socketserver.BaseRequestHandler):
    def handle(self):
        try:
            with self.server.context.wrap_socket(self.request, server_side=True) as connection:
                connection.settimeout(2)
                headers = connection.recv(8192)
                if headers.startswith(b'GET /probe '):
                    connection.sendall(b'HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nbackground-ok')
                    return
                connection.sendall(b'HTTP/1.1 100 Continue\r\n\r\n')
                start = time.monotonic()
                while not self.server.done.is_set():
                    if self.server.stall and time.monotonic() - start > .5:
                        # Leave TLS connected but stop draining the send buffers.
                        self.server.done.wait(.05)
                    elif not connection.recv(65536):
                        break
        except (OSError, ssl.SSLError):
            pass


class UploadServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True
    request_queue_size = 256

    def handle_error(self, *unused):
        pass


if args.offline:
    loopback(True)

with tempfile.TemporaryDirectory(prefix='storage-background-test-') as directory, contextlib.ExitStack() as cleanup:
    test = Path(directory)
    # Normal tests reuse the caller's prefix; isolated trials use one fresh prefix.
    identity = pwd.getpwnam(os.environ.get('SUDO_USER', 'agent')) if os.geteuid() == 0 else None
    if identity:
        test.chmod(0o755)
        os.chown(test, identity.pw_uid, identity.pw_gid)
    entries = json.loads((build / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']) == source / 'storage/storage.cpp')
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(test / 'test.obj')
    command[command.index('-c') + 1] = str(source / 'tests/background_multi.cpp')
    subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = [arg for arg in shlex.split(link.split(' && ')[1])
               if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'test.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in runtime.glob('*.dll'):
        shutil.copy2(library, test / library.name)
    shutil.copytree(runtime / 'shader', test / 'shader')
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes',
                    '-keyout', str(test / 'fixture.key'), '-out', str(test / 'fixture.crt'),
                    '-subj', '/CN=localhost', '-days', '1'], check=True, capture_output=True)
    env = {**os.environ, 'WINEDEBUG': '-all'}
    credentials = {'user': identity.pw_uid, 'group': identity.pw_gid} if identity else {}
    if identity:
        env['HOME'] = identity.pw_dir
        # A wineserver from outside this namespace would use its original network.
        # Keep one separate prefix for all offline trials, then remove it.
        prefix = test / 'wine'
        prefix.mkdir()
        os.chown(prefix, identity.pw_uid, identity.pw_gid)
        env['WINEPREFIX'] = str(prefix)
        wineserver = shutil.which('wineserver') or '/usr/lib/x86_64-linux-gnu/wine/wineserver'
        cleanup.callback(lambda: subprocess.run([wineserver, '-k', '-w'], env=env, **credentials))

    def run(mode, port, fault=None):
        scheme = 'http' if mode in ('ownership', 'pause') else 'https'
        wine = ['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64',
                str(test / 'test.exe'), mode, f'{scheme}://127.0.0.1:{port}/']
        process = subprocess.Popen(wine, cwd=test, env=env, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, start_new_session=True, **credentials)

        cancelled = threading.Event()
        injected = queue.Queue()
        def inject():
            if cancelled.wait(1.5):
                return
            try:
                if fault == 'down':
                    loopback(False)
                elif fault == 'drop':
                    firewall(True)
            except BaseException as error:
                injected.put(error)
            else:
                injected.put(None)

        injector = None
        first_line = queue.Queue()
        reader = threading.Thread(target=lambda: first_line.put(process.stdout.readline()), daemon=True)
        reader.start()
        try:
            # READY is emitted after Wine initialization and before the measured GUI loop.
            line = first_line.get(timeout=90)
            reader.join()
            print(line, end='', flush=True)
            if fault:
                assert line.startswith('READY:'), line
                injector = threading.Thread(target=inject, daemon=True)
                injector.start()
            stdout, stderr = process.communicate(timeout=25)
            print(stdout, end='', flush=True)
            if process.returncode:
                raise RuntimeError(f'{mode}/{fault} failed ({process.returncode}): {stderr}')
            if fault:
                error = injected.get(timeout=1)
                if error:
                    raise error
        except BaseException:
            import signal
            with contextlib.suppress(ProcessLookupError):
                os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
            raise
        finally:
            cancelled.set()
            if injector:
                injector.join()
            reader.join(timeout=1)
            if fault:
                firewall(False)
                loopback(True)

    http = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Response)
    threading.Thread(target=http.serve_forever, daemon=True).start()
    try:
        run('ownership', http.server_port)
        run('pause', http.server_port)
    finally:
        http.shutdown()
        http.server_close()

    trials = ('down', 'drop') * 3 if args.offline else (None,)
    for trial, fault in enumerate(trials, 1):
        server = UploadServer(('127.0.0.1', 0), Upload)
        server.context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server.context.load_cert_chain(test / 'fixture.crt', test / 'fixture.key')
        server.done = threading.Event()
        server.stall = not args.offline
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            print(f'TRIAL {trial}: {fault or "TLS backpressure"}', flush=True)
            run('stalled' if fault else 'stalled-pause', server.server_address[1], fault)
        finally:
            server.done.set()
            server.shutdown()
            server.server_close()
