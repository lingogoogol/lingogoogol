#!/usr/bin/env python3
"""Run versioned metadata and transfer integration against an isolated local Drive."""
import json
import os
import random
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

source=Path(__file__).resolve().parents[1]
build=source.parent/'build/x64-win32-pe-mingw_Release'
runtime=source.parent/'install/x64-win32-pe-mingw_Release'

def blob(name, content):
    return {'name':json.dumps(name),'description':json.dumps(content)}
def folder(name,parent,children):
    return blob({'name':name,'parent':parent,'parent_account':'fixture' if parent!='null' else 'null','next':'null','next_account':'null'},children)
def link(id,type,name):return {'id':id,'account':'fixture','type':type,'name':name}

class Handler(BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def log_message(self,*args):pass
    def resource(self,method,path,data=b''):
        id=urlparse(path).path.rsplit('/',1)[-1]
        with self.server.lock:
            if method=='GET':return (200,self.server.blobs[id].copy()) if id in self.server.blobs else (404,{})
            if method=='PATCH':
                body=json.loads(data)
                self.server.blobs.setdefault(id,{}).update(body)
                if id=='root' and 'name' in body and not self.server.failed_once:
                    self.server.failed_once=True
                    return 503, {'error':'lost-response fixture after applying metadata'}
                return 200,{'id':id}
            if method=='DELETE':return (204,{}) if self.server.blobs.pop(id,None) is not None else (404,{})
        raise AssertionError(method)
    def respond(self,status,body):
        body=body if isinstance(body,bytes) else json.dumps(body).encode()
        self.send_response(status);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
    def do_GET(self):self.respond(*self.resource('GET',self.path))
    def do_DELETE(self):self.respond(*self.resource('DELETE',self.path))
    def read_body(self):
        if self.headers.get('Transfer-Encoding','').lower()!='chunked':return self.rfile.read(int(self.headers.get('Content-Length',0)))
        data=b''
        while True:
            size=int(self.rfile.readline().strip().split(b';')[0],16)
            if not size:self.rfile.readline();return data
            data+=self.rfile.read(size);self.rfile.read(2)
    def do_PATCH(self):self.respond(*self.resource('PATCH',self.path,self.read_body()))
    def do_POST(self):
        body=self.read_body()
        boundary=self.headers['Content-Type'].split('boundary=')[1].strip('"')
        responses=[]
        for part in body.split(('--'+boundary).encode())[1:-1]:
            outer,request=part.strip(b'\r\n').split(b'\r\n\r\n',1)
            content_id=next(line.split(b':',1)[1].strip() for line in outer.split(b'\r\n') if line.lower().startswith(b'content-id:'))
            header,_,payload=request.partition(b'\r\n\r\n')
            method,path,_=header.split(b'\r\n')[0].decode().split(' ',2)
            status,value=self.resource(method,path,payload)
            result=json.dumps(value).encode() if status!=204 else b''
            responses.append(b'--reply\r\nContent-Type: application/http\r\nContent-ID: response-'+content_id.strip(b'<>')+b'\r\n\r\nHTTP/1.1 '+str(status).encode()+b' OK\r\nContent-Type: application/json\r\n\r\n'+result+b'\r\n')
        result=b''.join(responses)+b'--reply--\r\n'
        self.send_response(200);self.send_header('Content-Type','multipart/mixed; boundary=reply');self.send_header('Content-Length',str(len(result)));self.end_headers();self.wfile.write(result)

with tempfile.TemporaryDirectory(prefix='storage-versioned-') as directory:
    test=Path(directory)
    commands=json.loads((build/'compile_commands.json').read_text())
    for original,replacement,output in [('storage/storage.cpp','versioned_storage.cpp','test.obj'),('storage/google/easy/.cpp','mock_google_easy.cpp','mock.obj')]:
        command=shlex.split(next(e['command'] for e in commands if Path(e['file'])==source/original))
        command[command.index('-o')+1]=str(test/output);command[command.index('-c')+1]=str(source/'tests'/replacement)
        subprocess.run(command,cwd=build,check=True)
    link_command=subprocess.check_output(['ninja','-t','commands','storage.exe'],cwd=build,text=True).splitlines()[-1]
    command=[x for x in shlex.split(link_command.split(' && ')[1]) if x not in ('-mwindows','-municode') and not x.startswith('-Wl,--out-implib')]
    command[command.index('-o')+1]=str(test/'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')]=str(test/'test.obj')
    command[command.index('storage/google/easy/libstorage.google.easy.a')]=str(test/'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command,cwd=build,check=True)
    for p in (runtime/'bin').glob('*.dll'):shutil.copy2(p,test/p.name)
    for name in ['font','shader']:shutil.copytree(runtime/name,test/name)
    (test/'root').mkdir();(test/'input/nested').mkdir(parents=True);(test/'input/empty').mkdir()
    (test/'input/nested/payload.bin').write_bytes(random.Random(42).randbytes(1900000))
    assert (test/'input/nested/payload.bin').stat().st_size==1900000
    (test/'input/empty.bin').write_bytes(b'')
    (test/'queue').write_text(json.dumps([f'reserve-{i}' for i in range(20000)]))
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True;server.lock=threading.Lock();server.failed_once=False
    server.blobs={'root':folder('root','null',[link('old','folder','old'),link('dest','folder','dest')]),'dest':folder('dest','root',[]),'old':folder('old','root',[link('old-file','file','old.bin')]),
        'old-file':blob({'name':'old.bin','size':3,'parent':'old','parent_account':'fixture','next':'null','next_account':'null','account':'fixture'},['old-chunk']),
        'old-chunk':blob({'parent':'old-file','parent_account':'fixture','account':'fixture'},['old-data']),
        'old-data':{'name':json.dumps({'parent':'old-chunk','parent_account':'fixture'}),'description':'b2xk'}}
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        result=subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24','python3',str(source/'tests/versioned_storage_gui_driver.py'),str(test/'test.exe')],cwd=test,env={**os.environ,'WINEDEBUG':'-all','STORAGE_GPU_TEST_PORT':str(server.server_port)},capture_output=True,text=True,timeout=120)
        print(result.stdout,end='')
        if result.returncode:
            shutil.copytree(test,'/tmp/storage-versioned-failed',dirs_exist_ok=True)
            raise RuntimeError(f'exit {result.returncode}: {result.stderr}')
        assert server.failed_once, 'Lost-response retry was not exercised'
        assert not any('uncaught exception:' in p.read_text(errors='replace') for p in (test/'logfile').glob('*'))
        assert any('finished_all!' in p.read_text(errors='replace') for p in (test/'logfile').glob('*'))
        compare=runtime/'bin/compare.exe'
        for left,right,expected in [('input/nested/payload.bin','download.bin',True),('input/nested/payload.bin','different.bin',False)]:
            if not expected:(test/right).write_bytes(b'x'*1900000)
            # compare.exe takes interactive paths and can return zero for unequal files.
            result=subprocess.run(['wine64',str(compare)],input=f'{left}\n{right}\n',cwd=test,env={**os.environ,'WINEDEBUG':'-all'},capture_output=True,text=True,timeout=20)
            print(result.stdout,end='')
            assert result.returncode==0, result.stderr
            assert ('檔案內容完全相同' in result.stdout) if expected else ('位元組處不同' in result.stdout), result.stdout
    finally:server.shutdown();server.server_close()
