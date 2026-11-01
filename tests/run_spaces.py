#!/usr/bin/env python3
"""Exercise spaces against account-isolated local Drive, never real credentials."""
import base64
from collections import Counter
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import threading
from urllib.parse import urlsplit, parse_qs

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'
addresses = [f'user{i}@storage-ai-project{project}.iam.gserviceaccount.com' for project in (1, 2) for i in range(2)]


def compile_source(original, replacement, output):
    entries = json.loads((build / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']) == source / original)
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(output)
    command[command.index('-c') + 1] = str(replacement)
    subprocess.run(command, cwd=build, check=True)


def link(output, entry, easy, gui=False):
    text = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = shlex.split(text.split(' && ')[1])
    if not gui:
        command = [arg for arg in command if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(output)
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(entry)
    command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(easy)
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)


def kind(name):
    if 'next' in name:
        return 'file' if 'size' in name else 'folder'
    return 'chunk' if 'account' in name else 'data'


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *unused):
        pass

    def body(self):
        if self.headers.get('Transfer-Encoding') == 'chunked':
            data = b''
            while True:
                line = self.rfile.readline().strip()
                if not line: raise ConnectionResetError('Interrupted fixture upload')
                count = int(line.split(b';')[0], 16)
                if not count:
                    self.rfile.readline()
                    return data
                data += self.rfile.read(count)
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

    def operation(self, method, path, body=b'', batch=False):
        identifier = urlsplit(path).path.rsplit('/', 1)[-1]
        address = self.headers.get('X-Fixture-Account')
        with self.server.lock:
            try:
                if method == 'POST':
                    self.server.serial += 1
                    identifier = 'new-' + str(self.server.serial)
                    self.server.owners[identifier] = address
                    self.server.blobs[identifier] = json.loads(body)
                    return 200, {'id': identifier}
                if identifier not in self.server.owners:
                    raise AssertionError('Unreserved ID ' + identifier)
                assert self.server.owners[identifier] == address, ('Wrong identity', identifier, address)
                value = json.loads(body) if body else {}
                old = self.server.blobs.get(identifier)
                metadata = json.loads(value.get('name', (old or {}).get('name', '{}')))
                blob_kind = kind(metadata) if metadata else None
                assert not (batch and blob_kind == 'folder'), 'Folder blob sent inside batch: ' + identifier
                self.server.counts[method, batch] += 1
                if batch and method not in self.server.injected:
                    self.server.injected.add(method)
                    return 503, {'error': 'one transient fixture failure'}
                if method == 'GET':
                    return (200, old) if old else (404, {})
                if method == 'PATCH':
                    self.server.blobs.setdefault(identifier, {}).update(value)
                    return 200, self.server.blobs[identifier]
                if method == 'DELETE':
                    existed = self.server.blobs.pop(identifier, None)
                    return (204, {}) if existed else (404, {})
                raise AssertionError(method)
            except Exception as error:
                self.server.errors.append(str(error))
                return 400, {'error': 'Fixture rejected request'}

    def do_GET(self):
        if self.path == '/fixture/validate':
            with self.server.lock:
                for identifier, value in self.server.blobs.items():
                    name = json.loads(value.get('name') or '{}')
                    if 'parent' not in name:
                        continue
                    if name['parent'] != 'null':
                        assert name['parent_account'] == self.server.owners[name['parent']], (identifier, 'parent_account')
                    if 'next' in name and name['next'] != 'null':
                        assert name['next_account'] == self.server.owners[name['next']], (identifier, 'next_account')
                    blob_kind = kind(name)
                    if blob_kind == 'data':
                        continue
                    children = json.loads(value['description'])
                    assert len(children) <= 100, (identifier, 'oversized index')
                    for child in children:
                        child_id = child['id'] if blob_kind == 'folder' else child
                        address = child['account'] if blob_kind == 'folder' else name['account']
                        assert self.server.owners[child_id] == address, (identifier, 'child account', child_id)
                self.server.validations += 1
            return self.reply(200, {})
        if urlsplit(self.path).path == '/drive/v3/files':
            address = self.headers.get('X-Fixture-Account')
            query = parse_qs(urlsplit(self.path).query)
            offset = int(query.get('pageToken', ['0'])[0])
            with self.server.lock:
                files = [dict(id=identifier, name=value.get('name', '')) for identifier, value in sorted(self.server.blobs.items())
                         if self.server.owners[identifier] == address]
            result = {'files': files[offset:offset+100]}
            if offset+100 < len(files): result['nextPageToken'] = str(offset+100)
            return self.reply(200, result)
        self.reply(*self.operation('GET', self.path))

    def do_PATCH(self):
        self.reply(*self.operation('PATCH', self.path, self.body()))

    def do_DELETE(self):
        self.reply(*self.operation('DELETE', self.path))

    def do_POST(self):
        data = self.body()
        if not self.path.startswith('/batch'):
            with self.server.lock:
                self.server.serial += 1
                identifier = 'new-' + str(self.server.serial)
                self.server.owners[identifier] = self.headers.get('X-Fixture-Account')
                self.server.blobs[identifier] = json.loads(data)
            return self.reply(200, {'id': identifier})
        boundary = self.headers['Content-Type'].split('boundary=')[1]
        parts = []
        for part in data.decode().split('--' + boundary)[1:-1]:
            if '\r\n\r\n' not in part:
                self.server.errors.append('Malformed multipart: ' + repr((boundary,part[:300],data[:500])))
                return self.reply(400, {})
            header, request = part.lstrip('\r\n').split('\r\n\r\n', 1)
            number = re.search(r'Content-ID: (\d+)', header)[1]
            first, rest = request.split('\r\n', 1)
            method, path, unused = first.split(' ')
            body = request.split('\r\n\r\n', 1)[1].rstrip('\r\n') if '\r\n\r\n' in request else ''
            status, value = self.operation(method, path, body, batch=True)
            parts.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{number}\r\n\r\nHTTP/1.1 {status} Fixture\r\nContent-Type: application/json\r\n\r\n{json.dumps(value)}\r\n')
        assert len(parts) <= 100
        payload = (''.join(reversed(parts)) + '--fixture--\r\n').encode()
        self.reply(200, payload, 'multipart/mixed; boundary=fixture')


def seed(test):
    (test / 'key').mkdir()
    registry = {}
    owners = {'root': addresses[0], 'outside': addresses[0], 'peer-root': addresses[2],
              'not-linked': 'external@example.invalid'}
    (test / 'root').mkdir()
    (test / 'root/space-test').write_text(json.dumps({'account': addresses[0], 'id': 'root'}))
    (test / 'root/peer-space').write_text(json.dumps({'account': addresses[2], 'id': 'peer-root'}))
    (test / 'blob_queue').mkdir()
    for index, address in enumerate(addresses):
        path = test / f'key/program-{index}.json'
        path.write_text(json.dumps({'client_email': address, 'private_key': 'unused-test-key'}))
        registry[address] = 'key/' + path.name
        ids = [f'reserve-{index}-{i}' for i in range(30000)]
        owners.update(dict.fromkeys(ids, address))
        (test / 'blob_queue' / hashlib.sha256(address.encode()).hexdigest()).write_text(json.dumps(ids))
    (test / 'key/accounts.json').write_text(json.dumps(registry))
    (test / 'key/external.json').write_text(json.dumps({'client_email': 'external@example.invalid', 'private_key': 'never-read'}))
    settings = {'spaces': [{'id': 'space-test', 'name': '測試空間', 'accounts': addresses, 'signed_in': True},
                           {'id': 'peer-space', 'name': '共用帳戶空間', 'accounts': [addresses[2]], 'signed_in': True}],
                'selected_space': 'space-test', 'network_retries': 3, 'batch_request': True}
    (test / 'settings.json').write_text(json.dumps(settings))
    return owners


def seed_large_index(server):
    """Stress copy routing for two file pages, 101 chunks and 100 contents.

    Empty chunks keep this metadata-only fixture small; byte-for-byte transfer
    integrity is exercised separately by the uploaded real file.
    """
    chunks = [f'index-chunk-{i}' for i in range(101)]
    contents = [f'index-content-{i}' for i in range(100)]

    def put(identifier, owner, name, description):
        server.owners[identifier] = owner
        server.blobs[identifier] = {'name': json.dumps(name), 'description': description}

    for i, chunk in enumerate(chunks):
        put(chunk, addresses[2] if i < 100 else addresses[3],
            {'parent': 'index-main', 'parent_account': addresses[0], 'account': addresses[1] if i == 0 else 'null'},
            json.dumps(contents if i == 0 else []))
    for identifier in contents:
        put(identifier, addresses[1], {'parent': chunks[0], 'parent_account': addresses[2]}, 'eA')
    for identifier, owner, links, next_id, next_account, child_account in (
            ('index-main', addresses[0], chunks[:100], 'index-next', addresses[1], addresses[2]),
            ('index-next', addresses[1], chunks[100:], 'null', 'null', addresses[3])):
        put(identifier, owner, {'name': 'large-index', 'size': 100, 'parent': 'root',
            'parent_account': addresses[0], 'next': next_id, 'next_account': next_account, 'account': child_account},
            json.dumps(links))
    server.blobs['root']['description'] = json.dumps([
        {'id': 'index-main', 'account': addresses[0], 'type': 'file', 'name': 'large-index', 'size': 100}])


with tempfile.TemporaryDirectory(prefix='storage-spaces-') as directory:
    test = Path(directory)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.daemon_threads = True
    server.handle_error = lambda *args: None # Deliberate process termination closes active sockets.
    server.lock = threading.RLock()
    server.owners = seed(test)
    server.blobs = {'root': {'name': json.dumps({'name': 'root', 'parent': 'null', 'parent_account': 'null',
                                                'next': 'null', 'next_account': 'null'}), 'description': '[]'},
                    'peer-root': {'name': json.dumps({'name': 'root', 'parent': 'null', 'parent_account': 'null',
                                                     'next': 'null', 'next_account': 'null'}), 'description': '[]'},
                    'outside': {'name': '{}', 'description': 'must be cleared'},
                    'not-linked': {'name': '{}', 'description': 'must survive'}}
    server.serial = 0
    server.errors = []
    server.injected = set()
    server.counts = Counter()
    server.validations = 0
    seed_large_index(server)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        compile_source('storage/google/easy/.cpp', source / 'tests/mock_google_easy.cpp', test / 'easy.obj')
        compile_source('storage/storage.cpp', source / 'tests/spaces.cpp', test / 'test.obj')
        link(test / 'test.exe', test / 'test.obj', test / 'easy.obj')
        for dll in (runtime / 'bin').glob('*.dll'):
            shutil.copy2(dll, test / dll.name)
        for resource in ('font', 'shader'):
            shutil.copytree(runtime / resource, test / resource)
        tree = test / 'input/tree'
        tree.mkdir(parents=True)
        (tree / 'payload.bin').write_bytes(bytes(range(256)) * 16000)
        for i in range(205):
            (tree / f'empty-{i:03}.txt').write_bytes(b'')
        env = {**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)}
        for phase in ('upload-crash', 'resume', 'clear-resume'):
            run = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64', str(test / 'test.exe'), phase],
                                 cwd=test, env=env, capture_output=True, text=True, timeout=160)
            assert run.returncode == 0, run.stdout + run.stderr + repr(server.errors[:5])
            print(run.stdout, end='', flush=True)
            assert not server.errors, server.errors[:5]
        if '--direct' in sys.argv:
            assert not any(batch and count for (method, batch), count in server.counts.items()), server.counts
        assert 'outside' not in server.blobs, 'Clear failed to delete files outside the space root.'
        assert 'peer-root' not in server.blobs, 'Clear did not include other spaces sharing linked accounts.'
        assert server.blobs['not-linked']['description'] == 'must survive'
        assert all(identifier == 'not-linked' or value.get('description') == '[]' or not value.get('name')
                   for identifier, value in server.blobs.items()), 'Clear retained account data'
        for method in ('GET', 'PATCH', 'DELETE'):
            assert server.counts[method, False] > 0, server.counts
            if '--direct' not in sys.argv: assert server.counts[method, True] > 0, server.counts
        assert server.injected == (set() if '--direct' in sys.argv else {'GET', 'PATCH', 'DELETE'})
        assert server.validations >= 3, 'Metadata account fields were not validated.'
        logs = (test / 'logfile/spaces.txt').read_text(errors='replace')
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
        print('PASS: server-enforced account routing, <=100 same-user parts, direct folder requests, reversed/partial retries, clean shutdown clear restart and all linked-account data cleared.')
    except BaseException:
        evidence = Path('/tmp/storage-spaces-failure')
        shutil.rmtree(evidence, ignore_errors=True)
        shutil.copytree(test, evidence)
        print('Fixture errors:', server.errors[:5], flush=True)
        print('Synthetic failure evidence:', evidence, flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()
