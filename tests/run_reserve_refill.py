#!/usr/bin/env python3
"""Check real reserve workers against delayed/retried local batch responses."""
from collections import Counter
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import threading
from urllib.parse import urlsplit, parse_qs

source = Path(__file__).resolve().parents[1]
fixture_path = source/'tests/run_spaces.py'
fixture = {'__file__': str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],
             str(fixture_path), 'exec'), fixture)


class Server(ThreadingHTTPServer):
    request_queue_size = 128
    daemon_threads = True


class Handler(fixture['Handler']):
    def do_GET(self):
        if self.path.startswith('/fixture/release'):
            count = int(parse_qs(urlsplit(self.path).query)['count'][0])
            self.server.release.release(count)
            return self.reply(200, {})
        if self.path == '/fixture/status':
            with self.server.lock:
                value = {'active': self.server.active, 'maximum': self.server.maximum,
                         'attempts': self.server.attempts}
            return self.reply(200, value)
        self.server.errors.append('Unexpected request: '+self.path)
        self.reply(400, {})

    def do_POST(self):
        try:
            data = self.body()
            assert self.path == '/batch/drive/v3'
            assert self.headers.get('X-Fixture-Account') == 'user1@storage-ai-project1.iam.gserviceaccount.com'
            boundary = self.headers['Content-Type'].split('boundary=')[1]
            parts = data.decode().split('--'+boundary)[1:-1]
            with self.server.lock:
                self.server.attempts += 1
                self.server.active += 1
                self.server.maximum = max(self.server.maximum, self.server.active)
                attempt = self.server.attempts
                self.server.part_counts[len(parts)] += 1
            assert len(parts) == (1 if self.server.mode == 'empty-retry' and attempt > 1 and len(parts) == 1 else 100)
            if self.server.mode != 'empty-retry':
                assert self.server.release.acquire(timeout=20), 'Fixture response was never released'
            responses = []
            for i, part in enumerate(parts):
                header, request = part.lstrip('\r\n').split('\r\n\r\n', 1)
                identifier = re.search(r'Content-ID: (\d+)', header)[1]
                first = request.split('\r\n', 1)[0]
                assert first == 'POST /drive/v3/files?fields=id HTTP/1.1', first
                value = json.loads(request.split('\r\n\r\n', 1)[1])
                assert value == {'name': '', 'description': ''}
                status = 503 if self.server.mode == 'empty-retry' and attempt == 1 and i == 0 else 200
                value = {'error': 'retry once'} if status == 503 else {'id': f'created-{attempt}-{identifier}'}
                responses.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{identifier}\r\n\r\n'
                                 f'HTTP/1.1 {status} Fixture\r\nContent-Type: application/json\r\n\r\n{json.dumps(value)}\r\n')
            # Count the batch as finished before exposing its complete response
            # to the client, so the concurrency measurement has no response race.
            with self.server.lock:
                self.server.active -= 1
            self.reply(200, (''.join(reversed(responses))+'--fixture--\r\n').encode(), 'multipart/mixed; boundary=fixture')
        except Exception as error:
            self.server.errors.append(str(error))
            self.reply(400, {})


with tempfile.TemporaryDirectory(prefix='storage-reserve-refill-') as directory:
    test = Path(directory)
    fixture['compile_source']('storage/google/easy/.cpp', source/'tests/mock_google_easy.cpp', test/'easy.obj')
    fixture['compile_source']('storage/storage.cpp', source/'tests/reserve_refill.cpp', test/'test.obj')
    fixture['link'](test/'test.exe', test/'test.obj', test/'easy.obj')
    for dll in (fixture['runtime']/'bin').glob('*.dll'):
        shutil.copy2(dll, test/dll.name)
    for mode, initial, batches in [('full', 20000, 0), ('near-full', 19999, 1),
                                   ('partial-gap', 13450, 66), ('empty-retry', 0, 201)]:
        server = Server(('127.0.0.1', 0), Handler)
        server.lock = threading.Lock()
        server.release = threading.Semaphore(0)
        server.errors = []
        server.mode = mode
        server.attempts = server.active = server.maximum = 0
        server.part_counts = Counter()
        # Each scenario replaces the complete queue, including its tail edit.
        (test/'test-queue.tail').unlink(missing_ok=True)
        (test/'test-queue').write_text(json.dumps(['reserved']*initial))
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            run = subprocess.run(['wine64', str(test/'test.exe'), mode], cwd=test,
                env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                capture_output=True, text=True, timeout=30)
            assert run.returncode == 0, (run.stdout, run.stderr, server.errors)
            assert not server.errors, server.errors
            assert server.maximum <= 64, (mode, server.maximum)
            assert server.attempts == batches, (mode, server.attempts)
            if mode == 'partial-gap': assert server.maximum == 64
            if mode == 'empty-retry': assert server.part_counts == {100: 200, 1: 1}, server.part_counts
            print(run.stdout, end='', flush=True)
        finally:
            server.release.release(64)
            server.shutdown(); server.server_close()
    print('PASS: <=64 in-flight batches, rounded gaps including pending IDs, 20,000-create refill, background project/account balancing and retry counted once.')
