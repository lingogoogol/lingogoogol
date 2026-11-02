#!/usr/bin/env python3
"""Crash/resume actual schedulers; reject any repeat of confirmed completed work."""
from collections import Counter
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import time
from urllib.parse import urlsplit

source=Path(__file__).resolve().parents[1]
fixture_path=source/'tests/run_spaces.py'
fixture={'__file__':str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],str(fixture_path),'exec'),fixture)

class Handler(fixture['Handler']):
    def operation(self,method,path,body=b'',batch=False):
        identifier=urlsplit(path).path.rsplit('/',1)[-1]
        with self.server.lock:
            if (method,identifier) in self.server.forbidden:
                self.server.errors.append('Repeated completed work: '+method+' '+identifier)
                return 400,{}
            delay=self.server.delays.pop(identifier,0) if method=='GET' else 0
        if delay: time.sleep(delay)
        return super().operation(method,path,body,batch)

def checkpoints(test):
    return [json.loads(p.read_text()) for p in (test/'task.data').glob('*/*.progress')]

def forbid_upload(server,nodes):
    for s in nodes:
        for c in s.get('chunks',[]):
            if c.get('transfer_end'):server.forbidden.update(('PATCH',i) for i in c['content_id'])
            if c.get('load_end'):server.forbidden.add(('PATCH',c['id']))
        server.forbidden.update(('PATCH',p['id']) for p in s.get('pages',[]) if p['complete'])

with tempfile.TemporaryDirectory(prefix='storage-progress-') as directory:
    base=Path(directory)
    fixture['compile_source']('storage/google/easy/.cpp',source/'tests/mock_google_easy.cpp',base/'easy.obj')
    fixture['compile_source']('storage/storage.cpp',source/'tests/progress_resume.cpp',base/'test.obj')
    fixture['link'](base/'test.exe',base/'test.obj',base/'easy.obj')
    test=base/'current';test.mkdir()
    shutil.copy2(base/'test.exe',test/'test.exe')
    for dll in (fixture['runtime']/'bin').glob('*.dll'):shutil.copy2(dll,test/dll.name)
    for resource in ('font','shader'):shutil.copytree(fixture['runtime']/resource,test/resource)
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
    server.handle_error=lambda *args:None
    server.lock=threading.RLock();server.owners=fixture['seed'](test)
    server.blobs={'root':{'name':json.dumps({'name':'root','parent':'null','parent_account':'null','next':'null','next_account':'null'}),'description':'[]'}}
    server.serial=0;server.errors=[];server.injected=set();server.counts=Counter();server.forbidden=set();server.delays={}
    tree=test/'input/tree';tree.mkdir(parents=True)
    (tree/'empty-folder').mkdir()
    (tree/'payload.bin').write_bytes(bytes(range(256))*32000)
    for i in range(16):(tree/f'empty-{i:03}.txt').touch()
    many=test/'input/many';many.mkdir()
    for i in range(1945):(many/f'file-{i:04}.txt').write_text('x')
    threading.Thread(target=server.serve_forever,daemon=True).start()
    env={**os.environ,'WINEDEBUG':'-all','WINEPREFIX':str(base/'wine'), 'STORAGE_GPU_TEST_PORT':str(server.server_port)}
    try:
        for phase in ('concurrent','upload-crash','upload-resume','download-crash','download-resume'):
            if phase=='upload-resume':
                nodes=checkpoints(test);forbid_upload(server,nodes)
                assert len(server.forbidden)>100,'Crash fixture did not persist completed work'
                for p in (test/'task.data').glob('*/*.response'):p.unlink()
            if phase=='download-crash':
                server.forbidden.clear()
                file_id=next(i for i,b in server.blobs.items() if json.loads(b.get('name') or '{}').get('size')==8192000)
                chunks=json.loads(server.blobs[file_id]['description'])
                for index,delay in ((0,.3),(1,2),(3,1.5)):
                    ids=json.loads(server.blobs[chunks[index]]['description']);server.delays[ids[0]]=delay
            if phase=='download-resume':
                nodes=checkpoints(test);keep=set();has_prefix=has_out_of_order=False
                for s in nodes:
                    if 'filesystem_progress' not in s:continue
                    has_prefix|=bool(s['written'])
                    server.forbidden.add(('GET',s['source_id']))
                    for i,c in enumerate(s['chunks']):
                        if c['loaded']:server.forbidden.add(('GET',c['id']))
                        if c['received']:
                            server.forbidden.update(('GET',v) for v in c['content_id'])
                            if i>=s['filesystem_progress']:keep.add(c['response_key']+'.response');has_out_of_order=True
                assert has_prefix and has_out_of_order,'Missing durable prefix or out-of-order result'
                for p in (test/'task.data').glob('*/*.response'):
                    if p.name not in keep:p.unlink()
                with (test/'output/payload.bin').open('ab') as f:f.write(b'uncheckpointed tail')
            result=subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24','wine64',str(test/'test.exe'),phase],
                                  cwd=test,env=env,capture_output=True,text=True,timeout=120)
            assert result.returncode==0,result.stdout+result.stderr+repr(server.errors[:5])
            assert not server.errors,server.errors[:5]
            print(phase+' PASS',flush=True)
    except BaseException:
        evidence=Path('/tmp/storage-progress-failure');shutil.rmtree(evidence,ignore_errors=True);shutil.copytree(test,evidence)
        print('Synthetic evidence:',evidence,flush=True);raise
    finally:
        server.shutdown();server.server_close()
        server_path='/usr/lib/x86_64-linux-gnu/wine/wineserver'
        subprocess.run([server_path,'-k'],env=env,capture_output=True)
        subprocess.run([server_path,'-w'],env=env,capture_output=True)
