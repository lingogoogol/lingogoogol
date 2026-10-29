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
from urllib.parse import parse_qs, urlparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--scroll-only', action='store_true', help='Run only the production directory scrolling scenario.')
parser.add_argument("--pool-only", action="store_true", help="Run shared-pool concurrency and metadata-only move checks.")
parser.add_argument('--unlink-only', action='store_true', help='Run early unlink visibility and deletion retry checks.')
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
    # More than 64 full batches exercise all physical slots after batching.
    count = 6500
    file = entry('pool-file', 'file', 'pool')
    file['size'] = count * 3
    result = {'root': blob('root', 'null', [file, entry('tiny-file', 'file', 'tiny')]),
              'destination': blob('destination', 'root', []),
              'pool-file': blob('pool', 'root', [f'pool-chunk-{i}' for i in range(count // 100)], file=True),
              'tiny-file': blob('tiny', 'root', [], file=True)}
    metadata = json.loads(result['pool-file']['name'])
    metadata['size'] = count * 3
    result['pool-file']['name'] = json.dumps(metadata)
    expected = b''
    for chunk in range(count // 100):
        result[f'pool-chunk-{chunk}'] = {'name': json.dumps({'parent': 'pool-file'}),
            'description': json.dumps([f'pool-data-{i}' for i in range(chunk*100, (chunk+1)*100)])}
        for i in range(chunk*100, (chunk+1)*100):
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
                if identifier.startswith('reserve-') and identifier in self.reserved_ids():
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

    def reserved_ids(self):
        path = self.server.test / 'blob_queue'
        stamp = path.stat().st_mtime_ns
        if stamp != getattr(self.server, 'queue_stamp', None):
            self.server.queue_ids = set(json.loads(path.read_text()))
            self.server.queue_stamp = stamp
        return self.server.queue_ids

    def checked_resource(self, method, path, body=b''):
        identifier = urlparse(path).path.rsplit('/', 1)[-1]
        fields = parse_qs(urlparse(path).query).get('fields', [''])[0]
        if self.server.phase.startswith('unlink-'):
            target = 'visible-folder' if self.server.phase == 'unlink-folder' else 'visible-file'
            with self.server.lock:
                linked = any(item['id'] == target for page in ('root', 'root2') if page in self.server.blobs
                             for item in json.loads(self.server.blobs[page]['description']))
            if method == 'GET' and identifier == 'elsewhere':
                return 503, {}
            if method == 'GET' and identifier == 'root' and not linked and not (self.server.test / 'unlink-release').exists():
                return 503, {}
            if method == 'DELETE':
                if linked:
                    self.server.violations.append('Content DELETE started before unlinking its parent entry.')
                if not (self.server.test / 'unlink-observed').exists():
                    self.server.violations.append('Content DELETE started before the visible row was retired.')
                deadline = time.monotonic() + 20
                while not (self.server.test / 'unlink-release').exists() and time.monotonic() < deadline:
                    time.sleep(.01)
                if not (self.server.test / 'unlink-release').exists():
                    self.server.violations.append('GUI did not observe unlink while content DELETE was blocked.')
                with self.server.lock:
                    fail = not self.server.unlink_failure
                    self.server.unlink_failure = True
                if fail:
                    return 503, {}
        if self.server.phase == 'pool':
            marker = self.server.test / 'pool-stage'
            stage = marker.read_text() if marker.exists() else ''
            with self.server.lock:
                key = (stage, method, identifier, fields)
                self.server.calls[key] = self.server.calls.get(key, 0) + 1
                if stage == 'retry' and method == 'GET' and identifier == 'tiny-file' and not self.server.retry_failures:
                    self.server.retry_failures += 1
                    return 503, {}
                if stage == 'move' and identifier.startswith(('pool-data-', 'pool-chunk-')):
                    self.server.violations.append('Move touched file contents: ' + identifier)
                failure = None
                if stage == 'copy' and method == 'GET' and identifier == 'pool-data-42' and fields == 'description':
                    failure = 'copy-read'
                if stage == 'copy' and method == 'PATCH' and json.loads(body).get('description') == base64.urlsafe_b64encode((43).to_bytes(3,'little')).decode():
                    failure = 'copy-write'
                if stage == 'delete' and method == 'DELETE' and identifier == 'pool-data-43':
                    failure = 'delete'
                if failure and failure not in self.server.failed_once:
                    self.server.failed_once.add(failure)
                    if failure == 'copy-write': self.server.failed_copy_id = identifier
                    return 503, {}
                if stage == 'delete' and method == 'DELETE' and identifier == 'pool-data-42':
                    # Simulate an already-applied DELETE with a lost response.
                    self.server.blobs.pop(identifier, None)
                    return 404, {}
        return self.resource(method, path, body)

    def do_GET(self):
        if self.path == '/fixture/status':
            with self.server.lock:
                return self.respond(200, self.server.blobs.copy())
        self.respond(*self.checked_resource('GET', self.path))

    def do_PATCH(self):
        self.respond(*self.checked_resource('PATCH', self.path, self.body()))

    def do_DELETE(self):
        self.respond(*self.checked_resource('DELETE', self.path))

    def do_POST(self):
        body = self.body().decode()
        raw = re.findall(r'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--)', body, re.S)
        assert 0 < len(raw) <= 100, 'Invalid batch part count.'
        parts = []
        for identifier, request in raw:
            header, _, payload = request.partition('\r\n\r\n')
            method, path, *_ = header.splitlines()[0].split(' ')
            parts.append((identifier, method, path, payload.encode()))
        category = None
        stage = (self.server.test / 'pool-stage').read_text() if self.server.phase == 'pool' else ''
        if stage == 'copy':
            if any(method == 'GET' and 'pool-data-' in path and
                   parse_qs(urlparse(path).query).get('fields') == ['name'] for _,method,path,_ in parts):
                category = 'scan'
            elif any('pool-data-' in path or method == 'PATCH' and 'reserve-' in path for _,method,path,_ in parts):
                category = 'copy'
        elif stage == 'delete' and any(method == 'DELETE' for _,method,_,_ in parts):
            category = 'delete'
        if self.server.phase == 'pool':
            with self.server.lock:
                self.server.batch_sizes.append(len(parts))
        if category:
            with self.server.lock:
                self.server.active[category] += 1
                self.server.peak[category] = max(self.server.peak[category], self.server.active[category])
                if self.server.active[category] == 64: self.server.gates[category].set()
            if not self.server.gates[category].wait(20):
                self.server.violations.append('Did not reach 64 concurrent batch ' + category + ' requests.')
                self.server.gates[category].set()
        try:
            first = category == 'delete' and any(path.endswith('/pool-file') for _,_,path,_ in parts)
            if category == 'copy':
                first = any(method == 'PATCH' and json.loads(json.loads(payload).get('name','{}')).get('name') == 'pool'
                            for _,method,_,payload in parts)
            if first:
                deadline = time.monotonic() + 25
                while self.server.done[category] < 100 and time.monotonic() < deadline: time.sleep(.005)
                if self.server.done[category] < 100:
                    self.server.violations.append('Parallel batch pipeline stalled behind its first response.')
                identifiers = json.loads((self.server.test / 'task').read_text())
                intents = [json.loads((self.server.test / 'task.data' / value / 'state').read_text()) for value in identifiers]
                key = 'copy_index' if category == 'copy' else 'delete_index'
                if any(intent.get(key, 0) != 0 for intent in intents if intent['kind'] == 'directory-' + category):
                    self.server.violations.append('Checkpoint skipped an unfinished prefix: ' + category)
            responses = []
            for identifier,method,path,payload in parts:
                status,value = self.checked_resource(method,path,payload)
                content = value.decode() if isinstance(value, bytes) else json.dumps(value)
                responses.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{identifier}\r\n\r\nHTTP/1.1 {status} OK\r\n\r\n{content}\r\n')
            # Google may return subresponses in an arbitrary order.
            self.respond(200, (''.join(reversed(responses)) + '--fixture--\r\n').encode(), 'multipart/mixed; boundary=fixture')
        finally:
            if category:
                with self.server.lock:
                    self.server.active[category] -= 1
                    self.server.done[category] += len(parts)


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
    (test / 'blob_queue').write_text(json.dumps([f'reserve-{i}' for i in range(40000)]))
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
        phases = ('unlink-file', 'unlink-folder', 'unlink-away') if args.unlink_only else (('pool',) if args.pool_only else
            (() if args.scroll_only else ('backend', 'copy-crash', 'resume-copy', 'move-crash', 'resume-move',
                'delete-crash', 'resume-delete', 'pool', 'unlink-file', 'unlink-folder', 'unlink-away')))
        for phase in phases:
            server.phase = phase
            if phase.startswith('unlink-'):
                server.unlink_failure = False
                for marker in ('unlink-observed', 'unlink-release'):
                    (test / marker).unlink(missing_ok=True)
                server.blobs = {
                    'root': blob('root', 'null', [entry('keep-file', 'file', 'shared')], 'root2'),
                    'root2': blob('root', 'null', [entry('visible-file', 'file', 'shared')]),
                    'keep-file': blob('shared', 'root', [], file=True),
                    'visible-file': blob('shared', 'root', [], file=True),
                }
                if phase == 'unlink-folder':
                    server.blobs['root2'] = blob('root', 'null', [entry('visible-folder', name='shared')])
                    server.blobs.update({
                        'visible-folder': blob('shared', 'root', [entry('visible-child')]),
                        'visible-child': blob('child', 'visible-folder', [entry('visible-leaf', 'file')]),
                        'visible-leaf': blob('leaf', 'visible-child', [], file=True),
                    })
            if phase == 'pool':
                server.blobs = pool_fixture(test)
                server.active = dict.fromkeys(('scan', 'copy', 'delete'), 0)
                server.peak = server.active.copy()
                server.done = server.active.copy()
                server.gates = {key: threading.Event() for key in server.active}
                server.retry_failures = 0
                server.calls = {}
                server.failed_once = set()
                server.batch_sizes = []

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
                assert all(f'pool-data-{i}' not in server.blobs for i in range(6500))
                assert max(server.batch_sizes) == 100, 'Directory requests were not batched.'
                assert server.failed_once == {'copy-read', 'copy-write', 'delete'}, server.failed_once
                for (stage, method, identifier, fields), count in server.calls.items():
                    if stage == 'copy' and method == 'PATCH' and identifier.startswith('reserve-'):
                        assert count == (2 if identifier == server.failed_copy_id else 1), (identifier, count)
                    if stage == 'delete' and method == 'DELETE':
                        assert count == (2 if identifier == 'pool-data-43' else 1), (identifier, count)
                print('PASS: 64 shared physical handles, concurrent discovery/copy/delete, mixed upload/download, metadata-only move, out-of-order checkpoints and independent content', flush=True)
            if phase.startswith('unlink-'):
                assert not server.violations, server.violations
                assert server.unlink_failure, 'Content deletion network retry was not exercised.'
                assert 'keep-file' in server.blobs, 'Same-named sibling was deleted.'
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
        if not args.pool_only and not args.unlink_only:
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
            server.blobs = {'root': blob('root', 'null', [entry('nav-0')])}
            for i in range(8):
                server.blobs[f'nav-{i}'] = blob(f'nav-{i}-' + 'long-folder-name-' * 5,
                    'root' if i == 0 else f'nav-{i-1}', [entry(f'nav-{i+1}')] if i < 7 else [])
            size_fixture = entry('size-fixture', 'file')
            size_fixture['size'] = 18446744073709551615
            sizes = [0, 1023, 1024, 1536, 1024**2-1, 1024**2, 1024**3-1, 1024**3]
            size_entries = [entry('nav-6'), size_fixture]
            for size in sizes:
                value = entry('size-' + str(size), 'file')
                value['size'] = size
                size_entries.append(value)
            server.blobs['nav-5']['description'] = json.dumps(size_entries)
            process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                'python3', str(source / 'tests/directory_gui_driver.py'), str(test / 'test.exe'), 'breadcrumbs'],
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
                raise RuntimeError('Breadcrumb GUI: ' + stdout + stderr)
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
