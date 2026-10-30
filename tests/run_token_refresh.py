#!/usr/bin/env python3
"""Test production OAuth signing/cache/retries with synthetic keys and local HTTP fixtures."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'

modes = ('batch-headers', 'recovery', 'temporary', 'invalid',
         'malformed', 'missing', 'lifetime', 'unavailable', 'queue-error', 'shutdown')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--modes', nargs='+', choices=modes, default=modes)
args = parser.parse_args()


class State:
    def __init__(self, mode):
        self.mode = mode
        self.lock = threading.Lock()
        self.tokens = self.required = 0
        self.denied = self.batch_a = self.batch_b = self.transient = 0
        self.revoked = 0
        self.barrier = threading.Event()
        self.done = threading.Event()
        self.violations = []

    def report(self):
        return {key: getattr(self, key) for key in
                ('tokens', 'denied', 'batch_a', 'batch_b', 'violations')}


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *unused):
        pass

    def body(self):
        if self.headers.get('Transfer-Encoding') == 'chunked':
            result = b''
            while True:
                size = int(self.rfile.readline().strip().split(b';')[0], 16)
                if not size:
                    self.rfile.readline()
                    return result
                result += self.rfile.read(size)
                self.rfile.read(2)
        return self.rfile.read(int(self.headers.get('Content-Length', 0)))

    def respond(self, status, data, content_type='application/json'):
        if not isinstance(data, bytes):
            data = json.dumps(data).encode()
        self.send_response(status)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Content-Type', content_type)
        self.end_headers()
        try:
            self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_GET(self):
        self.handle_request()

    def do_POST(self):
        self.handle_request()

    def do_PATCH(self):
        self.handle_request()

    def handle_request(self):
        state = self.server.state
        path = urllib.parse.urlparse(self.path).path
        body = self.body().decode()
        if path.startswith('/control/'):
            with state.lock:
                if path in ('/control/revoke', '/control/revoke-sync-multi'):
                    state.required = state.tokens + 1
                return self.respond(200, state.report())
        if path == '/token':
            form = urllib.parse.parse_qs(body, strict_parsing=True)
            valid_grant = form.get('grant_type') == ['urn:ietf:params:oauth:grant-type:jwt-bearer']
            valid_assertion = len(form.get('assertion', [])) == 1 and bool(form['assertion'][0])
            with state.lock:
                state.tokens += 1
                issued = state.tokens
                state.required = max(state.required, issued)
                if self.command != 'POST' or not valid_grant or not valid_assertion:
                    state.violations.append('Invalid token request.')
            if not valid_grant:
                return self.respond(400, {'error': 'unsupported_grant_type'})
            if not valid_assertion:
                return self.respond(400, {'error': 'invalid_request'})
            if state.mode == 'shutdown':
                state.done.wait(30)
                return self.respond(500, {})
            if state.mode in ('invalid', 'queue-error'):
                return self.respond(400, {'error': 'invalid_grant', 'error_description': 'sensitive-response-marker'})
            if state.mode == 'unavailable' or (state.mode == 'temporary' and issued < 3):
                return self.respond(503, {'error': 'unavailable'})
            if state.mode == 'malformed':
                return self.respond(200, b'{invalid-json')
            if state.mode == 'missing':
                return self.respond(200, {'expires_in': 3600})
            if state.mode == 'lifetime':
                return self.respond(200, {'access_token': 'fixture-token-invalid', 'expires_in': 0})
            time.sleep(.5)
            return self.respond(200, {'access_token': f'fixture-token-{issued}',
                                     'expires_in': 2 if state.mode == 'recovery' and issued == 1 else 3600})

        authorization = self.headers.get('Authorization', '')
        if path.startswith('/revoked/') and authorization == 'Bearer fixture-token-2':
            with state.lock:
                state.revoked += 1
                index = state.revoked
                if state.revoked == 16:
                    state.barrier.set()
            if not state.barrier.wait(5):
                state.violations.append('Concurrent old-token requests did not reach the fixture.')
            if index == 16:
                time.sleep(1.2)  # Arrives after the replacement token has been cached.
            return self.respond(401, {})

        if path.startswith('/batch') and state.mode == 'batch-headers':
            parts = re.findall(r'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--)', body, re.S)
            response = []
            for identifier, part in parts:
                headers, _, content = part.partition('\r\n\r\n')
                method, target, *_ = headers.split('\r\n')[0].split()
                name = urllib.parse.urlparse(target).path.rsplit('/', 1)[-1]
                if method in ('POST', 'PATCH'):
                    valid = 'Content-Type: application/json; charset=UTF-8\r\n' in headers + '\r\n'
                    valid = valid and 'Host: www.googleapis.com\r\n' in headers + '\r\n'
                    if not valid:
                        state.violations.append('Missing JSON Content-Type or malformed Host in batch upload.')
                    status, value = (200, {'id': 'fixture-created', **json.loads(content)}) if valid else (400, {})
                response.append(f'--fixture\r\nContent-Type: application/http\r\n'
                                f'Content-ID: response-{identifier}\r\n\r\n'
                                f'HTTP/1.1 {status} Response\r\nContent-Type: application/json\r\n\r\n'
                                + json.dumps(value) + '\r\n')
            return self.respond(200, (''.join(response) + '--fixture--\r\n').encode(),
                                'multipart/mixed; boundary=fixture')

        if path.startswith('/batch'):
            parts = re.findall(r'Content-ID: (\d+)\r\n\r\nGET [^\r\n]*/files/([ab])\?', body)
            if not parts:
                state.violations.append('Batch request body was lost.')
            response = []
            with state.lock:
                for identifier, name in parts:
                    if name == 'a':
                        state.batch_a += 1
                    else:
                        state.batch_b += 1
                    rejected = name == 'b' and state.batch_b == 1
                    if rejected:
                        state.required = state.tokens + 1
                    elif authorization != f'Bearer fixture-token-{state.required}':
                        state.violations.append('Batch retry reused rejected authorization.')
                    status = 401 if rejected else 503 if name == 'b' and state.batch_b == 3 else 200
                    response.append(f'--fixture\r\nContent-Type: application/http\r\n'
                                    f'Content-ID: response-{identifier}\r\n\r\n'
                                    f'HTTP/1.1 {status} Response\r\nContent-Type: application/json\r\n\r\n{{}}\r\n')
            if state.batch_b == 2:
                # Retain the old failed subresponse across a malformed retry;
                # it must not be interpreted as another rejection of the new token.
                return self.respond(200, b'invalid multipart frame', 'multipart/mixed')
            return self.respond(200, (''.join(response) + '--fixture--\r\n').encode(),
                                'multipart/mixed; boundary=fixture')

        with state.lock:
            if path == '/denied':
                state.denied += 1
                return self.respond(401, {})
            if path == '/sync' and authorization == 'Bearer fixture-token-4':
                state.required = 5
                return self.respond(401, {})
            if authorization != f'Bearer fixture-token-{state.required}':
                # A deliberate stale cache is expected only after explicit revocation.
                return self.respond(401, {})
            expected = None
            if path == '/expires':
                expected = ('POST', 'queued-before-expiry')
            elif path.startswith('/revoked/'):
                expected = ('PATCH', 'patch-body-' + path.rsplit('/', 1)[1])
            elif path == '/transient':
                expected = ('POST', 'preserve-post-body')
            elif path == '/sync':
                expected = ('PATCH', 'preserve-sync-patch')
            elif path == '/sync-multi':
                expected = ('POST', 'sync-multi-body')
            if expected and (self.command, body) != expected:
                state.violations.append('Retry changed HTTP method or payload: ' + path)
            if path == '/transient':
                state.transient += 1
                if state.transient == 1:
                    return self.respond(503, {})
        return self.respond(200, b'ok')


with tempfile.TemporaryDirectory(prefix='storage-token-test-') as directory:
    test = Path(directory)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    port = server.server_port
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        # Replace only the OAuth endpoint in an otherwise unchanged production
        # implementation. The production API/configuration gains no test bypass.
        account = (source / 'storage/google/account/.cpp').read_text()
        assert account.count('https://oauth2.googleapis.com/token') == 1
        (test / 'account.cpp').write_text(account.replace('https://oauth2.googleapis.com/token',
                                                        f'http://127.0.0.1:{port}/token'))
        entries = json.loads((build / 'compile_commands.json').read_text())
        for original, replacement, output in (
            ('storage/storage.cpp', source / 'tests/token_refresh.cpp', test / 'test.obj'),
            ('storage/google/account/.cpp', test / 'account.cpp', test / 'account.obj'),
        ):
            entry = next(item for item in entries if Path(item['file']) == source / original)
            command = shlex.split(entry['command'])
            command[command.index('-o') + 1] = str(output)
            command[command.index('-c') + 1] = str(replacement)
            subprocess.run(command, cwd=build, check=True)
        link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
        command = [arg for arg in shlex.split(link.split(' && ')[1])
                   if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
        command[command.index('-o') + 1] = str(test / 'test.exe')
        command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'test.obj')
        command[command.index('storage/google/account/libstorage.google.account.a')] = str(test / 'account.obj')
        command.append('storage/google/account/CMakeFiles/storage.google.account.dir/.cppm.obj')
        subprocess.run(command, cwd=build, check=True)
        for library in runtime.glob('*.dll'):
            shutil.copy2(library, test / library.name)
        shutil.copytree(runtime / 'shader', test / 'shader')
        subprocess.run(['openssl', 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048',
                        '-out', str(test / 'fixture.key')], check=True, capture_output=True)
        env = {**os.environ, 'WINEDEBUG': '-all'}
        for mode in args.modes:
            server.state = State(mode)
            (test / 'test-queue').write_text('[]')
            try:
                result = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64',
                                         str(test / 'test.exe'), mode, f'http://127.0.0.1:{port}'],
                                        cwd=test, env=env, capture_output=True, text=True, timeout=40)
                print(mode + ': ' + result.stdout, end='', flush=True)
                if result.returncode:
                    # Requests use synthetic tokens, but suppress verbose curl headers anyway.
                    raise RuntimeError(f'{mode} failed with exit {result.returncode}.')
                report = server.state.report()
                assert not report['violations'], report['violations']
                expected = 7 if mode == 'recovery' else 3 if mode in ('temporary', 'unavailable') else 1
                assert report['tokens'] == expected, (mode, report['tokens'], expected)
                log = (test / 'logfile/token-refresh.txt').read_text(errors='replace')
                assert 'finished_all!' in log and 'uncaught exception:' not in log
                for secret in ('fixture-token-', 'assertion=', 'sensitive-response-marker'):
                    assert secret not in log, 'OAuth secrets leaked into failure logs.'
            finally:
                server.state.done.set()
    finally:
        server.shutdown()
        server.server_close()
