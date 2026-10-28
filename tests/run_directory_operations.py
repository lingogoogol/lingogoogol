#!/usr/bin/env python3
"""Exercise directory mutations and crash replay against an isolated local Drive."""
import argparse
import base64
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
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--scroll-only', action='store_true', help='Run only the production directory scrolling scenario.')
parser.add_argument("--pool-only", action="store_true", help="Run shared-pool concurrency and metadata-only move checks.")
args = parser.parse_args()

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'


def entry(identifier, kind='folder', name=None):
    result = {'id': identifier, 'type': kind, 'name': name or identifier}
    if kind == 'file':
        result['size'] = 0
    return result


def blob(name, parent, content, next_page='null', file=False):
    metadata = {'name': name, 'parent': parent, 'next': next_page}
    if file:
        metadata['size'] = 0
    return {'name': json.dumps(metadata), 'description': json.dumps(content)}


def fixture():
    result = {
        'root': blob('root', 'null', [entry(i) for i in ('destination', 'tree', 'conflict', 'malformed', 'restart')], 'root2'),
        'root2': blob('root', 'null', [entry('file', 'file'), entry('batched-0', 'file', 'batched')]),
        'destination': blob('destination', 'root', [entry('filler-' + str(i)) for i in range(99)] + [entry('other-conflict', name='conflict')]),
        'file': blob('file', 'root', [], 'file2', True),
        'file2': blob('file', 'root', [], file=True),
        'tree': blob('tree', 'root', [entry('child')], 'tree2'),
        'tree2': blob('tree', 'root', []),
        'child': blob('child', 'tree', [entry('nested', 'file')]),
        'nested': blob('nested', 'child', ['chunk'], file=True),
        'chunk': {'name': '{"parent":"nested"}', 'description': '["data"]'},
        'data': {'name': '{"parent":"chunk"}', 'description': 'YWJj'},
        'conflict': blob('conflict', 'root', []),
        'other-conflict': blob('conflict', 'destination', []),
        'malformed': blob('malformed', 'root', [entry('unrelated')]),
        'unrelated': blob('unrelated', 'outside', []),
        'restart': blob('restart', 'root', [entry('restart-child')]),
        'restart-child': blob('restart-child', 'restart', []),
    }

    # Cross the reservation batch boundary without thousands of HTTP requests.
    for i in range(101):
        result[f'batched-{i}'] = blob('batched', 'root', [], f'batched-{i+1}' if i < 100 else 'null', True)
    return result


def pool_fixture(test):
    file = entry('pool-file', 'file', 'pool')
    file['size'] = 384
    result = {'root': blob('root', 'null', [file, entry('tiny-file', 'file', 'tiny')]),
              'destination': blob('destination', 'root', []),
              'pool-file': blob('pool', 'root', ['pool-chunk-0', 'pool-chunk-1'], file=True),
              'tiny-file': blob('tiny', 'root', [], file=True)}
    metadata = json.loads(result['pool-file']['name'])
    metadata['size'] = 384
    result['pool-file']['name'] = json.dumps(metadata)
    expected = b''
    for chunk in range(2):
        result[f'pool-chunk-{chunk}'] = {'name': json.dumps({'parent': 'pool-file'}),
            'description': json.dumps([f'pool-data-{i}' for i in range(chunk*64, (chunk+1)*64)])}
        for i in range(chunk*64, (chunk+1)*64):
            content = i.to_bytes(3, 'little')
            expected += content
            result[f'pool-data-{i}'] = {'name': json.dumps({'parent': f'pool-chunk-{chunk}'}),
                'description': base64.urlsafe_b64encode(content).decode()}
    (test / 'expected.bin').write_bytes(expected)
    return result


class FixtureServer(ThreadingHTTPServer):
    request_queue_size = 128


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
                line = self.rfile.readline()
                if not line:
                    raise ConnectionResetError('Client exited during a request body.')
                count = int(line.strip().split(b';')[0], 16)
                if not count:
                    self.rfile.readline()
                    return result
                result += self.rfile.read(count)
                self.rfile.read(2)
        return self.rfile.read(int(self.headers.get('Content-Length', 0)))

    def respond(self, status, value, kind='application/json'):
        data = value if isinstance(value, bytes) else json.dumps(value).encode()
        self.send_response(status)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Content-Type', kind)
        self.end_headers()
        try:
            self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def resource(self, method, path, body=b''):
        identifier = urlparse(path).path.rsplit('/', 1)[-1]
        delay = False
        with self.server.lock:
            blobs = self.server.blobs
            if method == 'GET':
                if identifier not in blobs:
                    return 404, {}
                return 200, blobs[identifier].copy()
            if method == 'PATCH':
                # A reserved blob ID must already be removed from durable storage
                # before its first use, including newly allocated overflow pages.
                if identifier.startswith('reserve-') and identifier in json.loads((self.server.test / 'blob_queue').read_text()):
                    self.server.violations.append('ID used before durable removal: ' + identifier)
                value = json.loads(body)
                blobs.setdefault(identifier, {}).update(value)
                delay = (self.server.phase == 'move-crash' and identifier == 'restart' or
                         self.server.phase == 'copy-crash' and 'name' in value and
                         json.loads(value['name']).get('name') == 'restart-child')
                status, result = 200, {'id': identifier}
            elif method == 'DELETE':
                self.server.deleted.add(identifier)
                status = 204 if blobs.pop(identifier, None) is not None else 404
                result = b''
                delay = self.server.phase == 'delete-crash' and identifier == 'restart-child'
            else:
                raise AssertionError(method)
        if delay:
            # Apply the mutation but lose its response when the process exits.
            time.sleep(3)
        return status, result

    def dispatch(self, method, body=b''):
        category = None
        identifier = urlparse(self.path).path.rsplit('/', 1)[-1]
        stage = ''
        if self.server.phase == 'pool':
            marker = self.server.test / 'pool-stage'
            stage = marker.read_text() if marker.exists() else ''
            if stage == 'retry' and method == 'GET' and identifier == 'tiny-file' and not self.server.retry_failures:
                self.server.retry_failures += 1
                self.respond(503, {})
                return
            if stage == 'move' and identifier.startswith(('pool-data-', 'pool-chunk-')):
                self.server.violations.append('Move touched file contents: ' + identifier)
            if stage == 'copy' and method == 'GET' and identifier.startswith('pool-data-'):
                category = 'scan'
            elif stage == 'copy' and method == 'PATCH' and identifier.startswith('reserve-'):
                category = 'copy'
            elif stage == 'delete' and method == 'DELETE':
                category = 'delete'
        if category:
            with self.server.lock:
                self.server.active[category] += 1
                self.server.peak[category] = max(self.server.peak[category], self.server.active[category])
                if self.server.active[category] == 64:
                    self.server.gates[category].set()
            if not self.server.gates[category].wait(15):
                self.server.violations.append('Did not reach 64 concurrent ' + category + ' requests.')
                self.server.gates[category].set()
        try:
            # Keep the first response outstanding while over 100 later writes
            # finish. A checkpoint must not skip this unfinished prefix hole.
            first = category == 'delete' and identifier == 'pool-file'
            if category == 'copy':
                name = json.loads(json.loads(body).get('name', '{}'))
                first = name.get('name') == 'pool'
            if first:
                deadline = time.monotonic() + 20
                while self.server.done[category] < 100 and time.monotonic() < deadline:
                    time.sleep(.005)
                if self.server.done[category] < 100:
                    self.server.violations.append('Parallel pipeline stalled behind its first response.')
                identifiers = json.loads((self.server.test / 'task').read_text())
                intents = [json.loads((self.server.test / 'task.data' / value / 'state').read_text())
                           for value in identifiers]
                key = 'copy_index' if category == 'copy' else 'delete_index'
                if any(intent.get(key, 0) != 0 for intent in intents if intent['kind'] == 'directory-' + category):
                    self.server.violations.append('Checkpoint skipped an unfinished prefix: ' + category)
            self.respond(*self.resource(method, self.path, body))
        finally:
            if category:
                with self.server.lock:
                    self.server.active[category] -= 1
                    self.server.done[category] += 1

    def do_GET(self):
        if self.path == '/fixture/status':
            with self.server.lock:
                return self.respond(200, self.server.blobs.copy())
        self.dispatch('GET')

    def do_PATCH(self):
        self.dispatch('PATCH', self.body())

    def do_DELETE(self):
        self.dispatch('DELETE')

    def do_POST(self):
        body = self.body().decode()
        # Only the empty-file upload's batch metadata is expected here.
        parts = re.findall(r'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--)', body, re.S)
        responses = []
        for identifier, request in parts:
            header, _, payload = request.partition('\r\n\r\n')
            method, path, *_ = header.splitlines()[0].split(' ')
            status, value = self.resource(method, path, payload.encode())
            responses.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{identifier}\r\n\r\nHTTP/1.1 {status} OK\r\n\r\n' + json.dumps(value) + '\r\n')
        self.respond(200, (''.join(responses) + '--fixture--\r\n').encode(), 'multipart/mixed; boundary=fixture')


with tempfile.TemporaryDirectory(prefix='storage-directory-test-') as directory:
    test = Path(directory)
    commands = json.loads((build / 'compile_commands.json').read_text())
    for original, replacement, output in [('storage/storage.cpp', 'directory_operations.cpp', 'test.obj'),
                                           ('storage/google/easy/.cpp', 'mock_google_easy.cpp', 'mock.obj')]:
        command = shlex.split(next(item['command'] for item in commands if Path(item['file']) == source / original))
        command[command.index('-o') + 1] = str(test / output)
        command[command.index('-c') + 1] = str(source / 'tests' / replacement)
        subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    original_link = shlex.split(link.split(' && ')[1])
    original_link[original_link.index('storage/google/easy/libstorage.google.easy.a')] = str(test / 'mock.obj')
    original_link.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    original_link[original_link.index('-o') + 1] = str(test / 'production.exe')
    subprocess.run(original_link, cwd=build, check=True)
    command = [arg for arg in original_link if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'test.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in runtime.glob('*.dll'):
        shutil.copy2(library, test / library.name)
    for resource in ('font', 'shader'):
        shutil.copytree(runtime / resource, test / resource)
    (test / 'key.json').write_text(json.dumps({'client_email': 'mock@example.invalid', 'private_key': 'unused-fixture-key'}))
    (test / 'root').write_text('root')
    (test / 'blob_queue').write_text(json.dumps([f'reserve-{i}' for i in range(25000)]))
    (test / 'empty.bin').write_bytes(b'')
    server = FixtureServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    server.lock = threading.Lock()
    server.blobs = fixture()
    server.deleted = set()
    server.violations = []
    server.test = test
    server.phase = ''
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        phases = ('pool',) if args.pool_only else (() if args.scroll_only else
            ('backend', 'copy-crash', 'resume-copy', 'move-crash', 'resume-move', 'delete-crash', 'resume-delete', 'pool'))
        for phase in phases:
            server.phase = phase
            if phase == 'pool':
                server.blobs = pool_fixture(test)
                server.active = dict.fromkeys(('scan', 'copy', 'delete'), 0)
                server.peak = server.active.copy()
                server.done = server.active.copy()
                server.gates = {key: threading.Event() for key in server.active}
                server.retry_failures = 0

            process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64', str(test / 'test.exe'), phase],
                cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
            try:
                stdout, stderr = process.communicate(timeout=120 if phase == "pool" else 60)
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            print(stdout, end='', flush=True)
            if process.returncode != (73 if phase.endswith('crash') else 0):
                shutil.copytree(test, '/tmp/storage-directory-failed', dirs_exist_ok=True)
                raise RuntimeError(f'{phase}: exit {process.returncode}; {stderr}')
            if phase == 'pool':
                assert all(value == 64 for value in server.peak.values()), server.peak
                assert not server.violations, server.violations
                assert server.retry_failures == 1, 'Small-file network retry was not exercised.'
                assert all(identifier not in server.blobs for identifier in ('pool-file', 'pool-chunk-0', 'pool-chunk-1'))
                assert all(f'pool-data-{i}' not in server.blobs for i in range(128))
                print('PASS: 64 shared physical handles, concurrent discovery/copy/delete, mixed upload/download, metadata-only move, out-of-order checkpoints and independent content', flush=True)
            if phase == 'backend':
                assert {identifier for identifier in server.deleted if not identifier.startswith('reserve-')} == {'tree', 'tree2', 'child', 'nested', 'chunk', 'data', 'conflict'}, server.deleted
                assert 'unrelated' in server.blobs and 'malformed' in server.blobs
                for identifier in ('file', 'file2'):
                    assert json.loads(server.blobs[identifier]['name'])['parent'] == 'destination'
                root_entries = json.loads(server.blobs['root']['description']) + json.loads(server.blobs['root2']['description'])
                duplicates = [item for item in root_entries if item['name'] == '測試資料夾']
                assert len(duplicates) == 2 and len({item['id'] for item in duplicates}) == 2
                assert 'other-conflict' in server.blobs and 'conflict' not in server.blobs
                destination_entries = []
                page = 'destination'
                while page != 'null':
                    destination_entries += json.loads(server.blobs[page]['description'])
                    page = json.loads(server.blobs[page]['name'])['next']
                assert [item['id'] for item in destination_entries if item['name'] == 'conflict'] == ['other-conflict']
                assert {'parallel', 'empty.bin'} <= {item['name'] for item in root_entries}
                assert all(item['id'] != 'file' for item in root_entries)
                assert json.loads(server.blobs['destination']['name'])['next'].startswith('reserve-')
            if phase == 'resume-copy':
                assert 'restart' in server.blobs and 'restart-child' in server.blobs
                copies = [identifier for identifier, value in server.blobs.items()
                          if identifier.startswith('reserve-') and json.loads(value['name']).get('name') == 'restart']
                assert len(copies) == 1, 'Interrupted copy created duplicate root blobs.'
            if phase == 'resume-move':
                assert all(item['id'] != 'restart' for item in json.loads(server.blobs['root']['description']))
                assert json.loads(server.blobs['restart']['name'])['parent'] == 'destination'
            if phase.startswith('resume'):
                assert json.loads((test / 'task').read_text()) == [], 'Finished intent was not removed.'
        if not args.pool_only:
            if not args.scroll_only:
                assert 'restart' not in server.blobs and 'restart-child' not in server.blobs
                assert not server.violations, server.violations
                server.phase = 'ui'
                server.blobs = {
                    'root': blob('root', 'null', [entry('file', 'file'), entry('destination')]),
                    'file': blob('file', 'root', [], file=True),
                    'destination': blob('destination', 'root', []),
                }
                process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                    'python3', str(source / 'tests/directory_gui_driver.py'), str(test / 'production.exe')],
                    cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
                try:
                    stdout, stderr = process.communicate(timeout=70)
                finally:
                    if process.poll() is None:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
                print(stdout, end='', flush=True)
                if process.returncode:
                    shutil.copytree(test, '/tmp/storage-directory-failed', dirs_exist_ok=True)
                    raise RuntimeError('Directory GUI: ' + stderr)
            server.blobs = {'root': blob('root', 'null', [entry(f'folder-{i:02}', name=f'folder-{i:02}-' + 'long-name-' * 20) for i in range(40)])}
            server.blobs.update({f'folder-{i:02}': blob(f'folder-{i:02}', 'root', []) for i in range(40)})
            process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                'python3', str(source / 'tests/directory_gui_driver.py'), str(test / 'production.exe'), 'scroll'],
                cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
            try:
                stdout, stderr = process.communicate(timeout=60)
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            print(stdout, end='', flush=True)
            if process.returncode:
                raise RuntimeError('Directory scrolling GUI: ' + stderr)
            if not args.scroll_only:
                print('PASS: Unicode/invalid names, distinct same-named folders, same-name moves and deletion by ID, independent recursive copies and crash replay, paged moves, self/descendant rejection, recursive deletion, malformed parent protection, concurrent upload publication and durable ID reservations')
    finally:
        server.shutdown()
        server.server_close()
