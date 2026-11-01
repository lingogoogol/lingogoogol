#!/usr/bin/env python3
"""Test task batch toggling against gated local HTTP and real GUI clicks."""
from collections import Counter
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

source=Path(__file__).resolve().parents[1]
fixture_path=source/'tests/run_spaces.py'
fixture={'__file__':str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],str(fixture_path),'exec'),fixture)

class Handler(fixture['Handler']):
    def do_GET(self):
        if self.path.startswith('/fixture/release-'):
            (self.server.batch_gate if self.path.endswith('batch') else self.server.direct_gate).set()
            return self.reply(200,{})
        if self.path=='/fixture/reset-direct':
            self.server.direct_gate.clear()
            return self.reply(200,{})
        if self.path=='/fixture/stats':
            with self.server.lock: value={'held':self.server.held,'batches':self.server.batches}
            return self.reply(200,value)
        if self.path.startswith('/fixture/held/'):
            with self.server.lock:
                self.server.held+=1
                self.server.calls[self.path]+=1
            if not self.server.direct_gate.wait(60): return self.reply(500,{})
            # Retry only one child; its siblings must never be reissued.
            if self.path.endswith('/direct-a/7') and self.server.calls[self.path]==1:
                return self.reply(503,{'error':{'errors':[{'reason':'backendError'}]}})
            return self.reply(200,{'index':int(self.path.rsplit('/',1)[1])})
        super().do_GET()

    def operation(self,method,path,body=b'',batch=False):
        if path.startswith('/fixture/held/'):
            assert batch and method=='GET'
            return 200,{'index':int(path.rsplit('/',1)[1])}
        return super().operation(method,path,body,batch)

    def do_POST(self):
        if self.path.startswith('/batch'):
            with self.server.lock:self.server.batches+=1
            if not self.server.batch_gate.wait(60):return self.reply(500,{})
        super().do_POST()

interaction=r'''
import json,subprocess,sys,time
from pathlib import Path
p=subprocess.Popen(['wine64',sys.argv[1],'ui'],stdout=open('ui-output','w'),stderr=subprocess.DEVNULL)
def wait(fn,message):
    until=time.monotonic()+25
    while time.monotonic()<until:
        try:
            value=fn()
            if value:return value
        except (FileNotFoundError,json.JSONDecodeError,subprocess.CalledProcessError,IndexError):pass
        if p.poll() is not None:raise AssertionError(Path('ui-output').read_text())
        time.sleep(.05)
    raise AssertionError(message)
def state():return json.loads(Path('ui.json').read_text())
def command(*args):return subprocess.check_output(['xdotool',*map(str,args)],text=True).strip()
window=wait(lambda:command('search','--name','Batch mode regression').splitlines()[0],'No GUI')
command('windowfocus','--sync',window)
def click(x,y):command('mousemove','--window',window,x,y,'click','1');time.sleep(.15)
def open_settings():
    Path('open-settings').touch();return wait(lambda:state() if state()['dialog'] else None,'No settings dialog')
def checkbox(s):click((s['width']-720)//2+40,(s['height']-692)//2+568)
def accept(s,ok=True):click((s['width']-720)//2+(640 if ok else 520),(s['height']-692)//2+660)
def workers(s,value):
    click((s['width']-720)//2+150,(s['height']-692)//2+532)
    command('key','ctrl+a')
    command('key','BackSpace') if not value else command('type','--clearmodifiers','--',value)
try:
    s=open_settings();assert s['values'][-1]=='true'
    workers(s,'31');checkbox(s);wait(lambda:state()['values'][-1]=='false','Checkbox did not uncheck')
    accept(s,False);wait(lambda:not state()['dialog'],'Cancel did not close')
    assert state()['batch'] and state()['workers']==64 and not Path('settings.json').exists(),'Cancel applied settings'
    s=open_settings();assert s['values'][-1]=='true'
    assert s['values'][6]=='64','Cancel retained the worker edit'
    for invalid in ('0','-1','1.5','abc','18446744073709551616',''):
        workers(s,invalid);accept(s)
        assert state()['dialog'] and not Path('settings.json').exists(),'Invalid worker count accepted: '+repr(invalid)
    workers(s,'128');checkbox(s);accept(s)
    wait(lambda:not state()['batch'] and state()['workers']==128 and state()['pool']==128,'Worker count not applied')
    assert json.loads(Path('settings.json').read_text())['batch_request'] is False
    command('keydown','F3');time.sleep(.1)
    s=state();click(s['next']['x'],s['next']['y']);wait(lambda:state()['page']==1,'F3 did not paginate custom worker slots')
    command('keyup','F3')
    s=open_settings();assert s['values'][-1]=='false'
    checkbox(s);accept(s);wait(lambda:state()['batch'] and not state()['dialog'],'Batch mode not restored')
    assert json.loads(Path('settings.json').read_text())['batch_request'] is True
    assert state()['workers']==128,'Changing batch mode overwrote worker count'
    s=open_settings();assert s['values'][6]=='128','Worker field did not reload saved value'
    workers(s,'17');accept(s)
    wait(lambda:state()['workers']==17 and not state()['dialog'],'Count-only change did not apply')
    saved=json.loads(Path('settings.json').read_text())
    assert saved['worker_count']==17 and saved['batch_request'] is True
    Path('close-test').touch();assert p.wait(15)==0,Path('ui-output').read_text()
    print(Path('ui-output').read_text(),end='')
finally:
    if p.poll() is None:p.kill();p.wait()
'''

with tempfile.TemporaryDirectory(prefix='storage-batch-mode-') as directory:
    test=Path(directory)
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
    server.request_queue_size=512;server.handle_error=lambda *args:None
    server.lock=threading.RLock();server.held=server.batches=0;server.calls=Counter()
    server.direct_gate=threading.Event();server.batch_gate=threading.Event()
    server.errors=[]
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        fixture['compile_source']('storage/google/easy/.cpp',source/'tests/mock_google_easy.cpp',test/'easy.obj')
        fixture['compile_source']('storage/storage.cpp',source/'tests/batch_mode.cpp',test/'test.obj')
        fixture['link'](test/'test.exe',test/'test.obj',test/'easy.obj')
        for dll in (fixture['runtime']/'bin').glob('*.dll'):shutil.copy2(dll,test/dll.name)
        for resource in ('font','shader'):shutil.copytree(fixture['runtime']/resource,test/resource)
        env={**os.environ,'WINEDEBUG':'-all','STORAGE_GPU_TEST_PORT':str(server.server_port)}
        run=subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24','wine64',str(test/'test.exe')],cwd=test,env=env,capture_output=True,text=True,timeout=130)
        assert run.returncode==0,run.stdout+run.stderr
        print(run.stdout,end='',flush=True)
        assert server.batches==1
        assert server.calls['/fixture/held/direct-a/7']==2
        assert all(n==1 for path,n in server.calls.items() if '/direct-' in path and not path.endswith('/direct-a/7')),server.calls
        subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24','python3','-c',interaction,str(test/'test.exe'),'ui'],cwd=test,env=env,check=True,timeout=100)
        logs=(test/'logfile/batch-mode.txt').read_text(errors='replace')
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
    except BaseException:
        evidence=Path('/tmp/storage-batch-mode-failure');shutil.rmtree(evidence,ignore_errors=True);shutil.copytree(test,evidence)
        print('Synthetic evidence:',evidence,flush=True)
        raise
    finally:
        server.direct_gate.set();server.batch_gate.set();server.shutdown();server.server_close()
