#!/usr/bin/env python3
"""Crash/restart real Windows task scheduler against an isolated in-memory Drive."""
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

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'

class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def log_message(self, *unused): pass
    def handle(self):
        try: super().handle()
        except ConnectionResetError: pass
    def body(self):
        if self.headers.get('Transfer-Encoding') == 'chunked':
            content = b''
            while True:
                line=self.rfile.readline().strip().split(b';')[0]
                if not line: return content
                size = int(line, 16)
                if not size:
                    self.rfile.readline(); return content
                content += self.rfile.read(size); self.rfile.read(2)
        return self.rfile.read(int(self.headers.get('Content-Length', 0)))
    def respond(self, status, body, kind='application/json'):
        if not isinstance(body, bytes): body = json.dumps(body).encode()
        self.send_response(status); self.send_header('Content-Length', str(len(body)))
        self.send_header('Content-Type', kind); self.end_headers()
        try: self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError): pass
    def resource(self, method, path, body):
        identifier = urlparse(path).path.rsplit('/', 1)[-1]
        account=self.headers.get('X-Fixture-Account','mock@example.invalid')
        blobs=self.server.other if account=='second@example.invalid' else self.server.blobs
        if method=='GET' and identifier=='files':
            from urllib.parse import parse_qs
            start=int(parse_qs(urlparse(path).query).get('pageToken',['0'])[0])
            with self.server.lock: ids=sorted(blobs)
            result={'files':[{'id':name} for name in ids[start:start+100]]}
            if start+100<len(ids):result['nextPageToken']=str(start+100)
            return 200,result
        if method == 'GET':
            if identifier == 'retry-file':
                with self.server.lock:
                    self.server.retry += 1; failed = self.server.retry == 1
                if failed: return 503, {}
                return 200, {'name': json.dumps({'size':3, 'next':'null'}), 'description':'["retry-chunk"]'}
            if identifier == 'retry-chunk': return 200, {'description':'["retry-data"]'}
            if identifier == 'retry-data': return 200, {'description': base64.urlsafe_b64encode(b'ABC').decode()}
            if identifier == 'cancel-file': time.sleep(.5)
            if identifier == 'slow-folder': time.sleep(1)
            if identifier in ('fast-folder', 'slow-folder'): return 200, {'name':'{"next":"null"}', 'description':'[]'}
            with self.server.lock:
                self.server.gets[identifier] = self.server.gets.get(identifier,0)+1
                value = blobs.get(identifier)
            if value is None: return 404, {}
            return 200, value.copy()
        if method == 'PATCH':
            if self.server.phase=='account-switch' and account!='mock@example.invalid':
                self.server.violations.append('Old upload was sent to the new account.')
            value = json.loads(body)
            if 'description' in value and len(value['description']) > 20000:
                # The last upload chunk stays in flight until we have checkpointed a prefix.
                with self.server.lock:
                    self.server.upload_calls += 1; index = self.server.upload_calls
                if index > 100: time.sleep(.015)
            with self.server.lock:
                blobs.setdefault(identifier,{}).update(value)
            return 200, {'id':identifier}
        if method == 'DELETE':
            time.sleep(.005)
            if account!='mock@example.invalid':self.server.violations.append('Cleared the other account.')
            with self.server.lock: blobs.pop(identifier,None)
            return 204, {}
        if method == 'POST' and identifier == 'files':
            # Clearing restarts the real reserve worker after creating its root.
            # These are new empty blobs, distinct from the deleted inventory.
            with self.server.lock:
                self.server.reserves += 1
                identifier = f'reserve-{self.server.reserves}'
                blobs[identifier] = json.loads(body)
            return 200, {'id': identifier}
        raise RuntimeError((method,path))
    def do_GET(self):
        status, value = self.resource('GET',self.path,b'');self.respond(status,value)
    def do_PATCH(self):
        status, value = self.resource('PATCH',self.path,self.body());self.respond(status,value)
    def do_DELETE(self):
        status,value=self.resource('DELETE',self.path,b'');self.respond(status,b'')
    def do_POST(self):
        body=self.body().decode()
        if not body: return
        if urlparse(self.path).path=='/drive/v3/files':
            value=json.loads(body)
            account=self.headers.get('X-Fixture-Account','mock@example.invalid')
            blobs=self.server.other if account=='second@example.invalid' else self.server.blobs
            identifier='fresh-root'
            with self.server.lock:blobs[identifier]=value
            return self.respond(200,{'id':identifier})
        parts = re.findall(r'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--\+)', body, re.S)
        response=[]
        for identifier, request in parts:
            header, _, payload = request.partition('\r\n\r\n')
            method, path, *_ = header.splitlines()[0].split(' ')
            status,value=self.resource(method,path,payload)
            response.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{identifier}\r\n\r\nHTTP/1.1 {status} OK\r\n\r\n'+json.dumps(value)+'\r\n')
        if self.server.phase == 'download-crash' and parts:
            with self.server.lock:
                self.server.download_batches += 1; index=self.server.download_batches
            if index > 2: time.sleep(1)
        self.respond(200,(''.join(response)+'--fixture--\r\n').encode(),'multipart/mixed; boundary=fixture')

with tempfile.TemporaryDirectory(prefix='storage-task-test-') as directory:
    test=Path(directory)
    entries=json.loads((build/'compile_commands.json').read_text())
    for original,replacement,output in [('storage/storage.cpp','task_lifecycle.cpp','test.obj'),('storage/google/easy/.cpp','mock_google_easy.cpp','mock.obj')]:
        entry=next(e for e in entries if Path(e['file'])==source/original)
        command=shlex.split(entry['command']);command[command.index('-o')+1]=str(test/output)
        command[command.index('-c')+1]=str(source/'tests'/replacement)
        subprocess.run(command,cwd=build,check=True)
    link=subprocess.check_output(['ninja','-t','commands','storage.exe'],cwd=build,text=True).splitlines()[-1]
    command=[arg for arg in shlex.split(link.split(' && ')[1]) if arg not in ('-mwindows','-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o')+1]=str(test/'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')]=str(test/'test.obj')
    command[command.index('storage/google/easy/libstorage.google.easy.a')]=str(test/'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command,cwd=build,check=True)
    for library in runtime.glob('*.dll'): shutil.copy2(library,test/library.name)
    for resource in ('font','shader'): shutil.copytree(runtime/resource,test/resource)
    (test/'key.json').write_text(json.dumps({'client_email':'mock@example.invalid','private_key':'mock-only-key'}))
    (test/'second.json').write_text(json.dumps({'client_email':'second@example.invalid','private_key':'mock-only-key'}))
    (test/'second-root').write_text('second-root')
    import hashlib
    (test/('blob_queue.'+hashlib.sha256(b'second@example.invalid').hexdigest())).write_text(json.dumps([f'second-{i}' for i in range(25000)]))
    (test/'second.bin').write_bytes(b'second source'*10000)
    (test/'root').write_text('root'); (test/'blob_queue').write_text(json.dumps([f'blob-{i}' for i in range(25000)]))
    (test/'source.bin').write_bytes(bytes(range(256))*20000)
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
    server.lock=threading.Lock();server.blobs={'root':{'name':'{"next":"null"}','description':'[]'}}
    server.other={'second-root':{'name':'{"next":"null"}','description':'[]'}};server.violations=[]
    server.gets={};server.upload_calls=server.download_batches=server.retry=server.reserves=0;server.phase=''
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        for phase in ('upload-crash','resume-upload','download-crash','resume-download','account-switch','clear-crash','resume-clear'):
            server.phase=phase
            # xvfb-run can leave Wine children alive if only the wrapper is killed.
            # Give each trial its own process group so timeout cleanup is complete.
            process = subprocess.Popen(
                ['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64', str(test / 'test.exe'), phase],
                cwd=test, env={**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
            try:
                stdout, stderr = process.communicate(timeout=55)
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            result = subprocess.CompletedProcess(process.args, process.returncode, stdout, stderr)
            print(phase+': '+result.stdout,flush=True)
            expected=73 if phase.endswith('crash') else 0
            if result.returncode!=expected:
                shutil.copytree(test, '/tmp/storage-task-failed',dirs_exist_ok=True)
                raise RuntimeError(f'{phase}: exit {result.returncode}; {result.stderr[-1000:]}')
            if phase=='resume-upload':
                assert len(json.loads(server.blobs['root']['description']))==1, 'Duplicate upload publication.'
            if phase.endswith('crash') and phase!='clear-crash':
                index=json.loads((test/'task').read_text());assert len(index)==1
                saved=json.loads((test/'task.data'/index[0]/'state').read_text());assert saved['progress']>0
        assert 'fresh-root' in server.blobs and all(
            identifier == 'fresh-root' or identifier.startswith('reserve-') for identifier in server.blobs
        ), 'Clear did not remove the entire original account inventory.'
        assert 'second-root' in server.other, 'Clear affected a different account.'
        assert not server.violations, server.violations
        assert (test/'output.bin').read_bytes()==(test/'source.bin').read_bytes()
        print('PASS: forced process exits preserve blob reservations, upload publication and exact download bytes; no live cloud data')
    finally:server.shutdown();server.server_close()
