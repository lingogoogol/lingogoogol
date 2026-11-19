#!/usr/bin/env python3
"""Exercise frame pacing, native visibility and the real settings form without credentials."""
import os
from pathlib import Path
import shutil
import select
import socket
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
fixture_path = source / 'tests/run_spaces.py'
fixture = {'__file__': str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],
             str(fixture_path), 'exec'), fixture)

interaction = r'''
import json, subprocess, sys, time
from pathlib import Path
render = subprocess.run(['wine64', sys.argv[1], 'render'], capture_output=True, text=True, timeout=40)
print(render.stdout,end='',flush=True)
assert render.returncode==0,(render.returncode,render.stderr)
log=Path('logfile/render-settings.txt').read_text()
assert 'uncaught exception:' not in log and 'finished_all!' in log
Path('logfile/render-settings.txt').unlink()
p = subprocess.Popen(['wine64', sys.argv[1], 'ui'], stdout=open('ui-output', 'w'), stderr=subprocess.DEVNULL)
def command(*args):
    return subprocess.check_output(['xdotool', *map(str, args)], text=True).strip()
def state(): return json.loads(Path('ui.json').read_text())
def wait(fn, message):
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        try:
            value = fn()
            if value: return value
        except (FileNotFoundError, json.JSONDecodeError, IndexError, subprocess.CalledProcessError): pass
        if p.poll() is not None: raise AssertionError(Path('ui-output').read_text())
        time.sleep(.05)
    raise AssertionError(message)
window = wait(lambda: command('search', '--name', 'Render settings regression').splitlines()[0], 'No GUI')
command('windowfocus', '--sync', window)
def click(s, x, y):
    command('mousemove', '--window', window, (s['width']-720)//2+x, max(0,(s['height']-932)//2)+y, 'click', '1')
    time.sleep(.1)
def open_settings():
    Path('open-settings').touch()
    return wait(lambda: state() if state()['dialog'] else None, 'No settings dialog')
def field(s, index, value):
    click(s, 150, 124+68*index)
    command('key', 'ctrl+a')
    command('key', 'BackSpace') if not value else command('type', '--clearmodifiers', '--', value)
def accept(s, ok=True): click(s, 640 if ok else 520, 900)
try:
    s = open_settings()
    assert s['values'][8:10] == ['60','64']
    field(s,8,'17');field(s,9,'12');accept(s,False)
    wait(lambda: not state()['dialog'], 'Cancel did not close')
    assert state()['fps']==60 and state()['reserve']==64 and not Path('settings.json').exists()
    for index in (8,9):
        s=open_settings()
        assert s['values'][8:10]==['60','64'], 'Cancelled edit was retained'
        for invalid in ('0','-1','1.5','abc','18446744073709551616',''):
            field(s,index,invalid);accept(s)
            assert state()['dialog'] and not Path('settings.json').exists(), ('Invalid limit accepted',index,invalid)
        accept(s,False);wait(lambda: not state()['dialog'], 'Cancel did not close')
    s=open_settings();field(s,8,'17');field(s,9,'12');accept(s)
    wait(lambda: not state()['dialog'] and state()['fps']==17 and state()['reserve']==12, 'New limits not applied')
    saved=json.loads(Path('settings.json').read_text())
    assert saved['frame_rate_limit']==17 and saved['reserve_in_flight_limit']==12
    assert saved['worker_count']==64 and saved['batch_size']==100 and saved['batch_request'] is True
    s=open_settings();assert s['values'][8:10]==['17','12'];accept(s,False)
    wait(lambda: not state()['dialog'], 'Final cancel did not close')
    Path('close-test').touch();assert p.wait(timeout=15)==0
    print(Path('ui-output').read_text(),end='')
finally:
    if p.poll() is None: p.kill();p.wait()
'''

with tempfile.TemporaryDirectory(prefix='storage-render-settings-') as directory, socket.socket() as endpoint:
    endpoint.bind(('127.0.0.1',0));endpoint.listen()
    test = Path(directory)
    fixture['compile_source']('storage/google/easy/.cpp', source/'tests/mock_google_easy.cpp', test/'easy.obj')
    fixture['compile_source']('storage/storage.cpp', source/'tests/render_settings.cpp', test/'test.obj')
    fixture['link'](test/'test.exe', test/'test.obj', test/'easy.obj')
    for dll in (fixture['runtime']/'bin').glob('*.dll'):
        shutil.copy2(dll, test/dll.name)
    for resource in ('font','shader'):
        shutil.copytree(fixture['runtime']/resource,test/resource)
    (test/'driver.py').write_text(interaction)
    env = {**os.environ, 'WINEDEBUG':'-all','STORAGE_GPU_TEST_PORT':str(endpoint.getsockname()[1])}
    # Keep both Wine processes on one display; Wine may retain a display connection
    # briefly after a process exits, so switching Xvfb instances here is unreliable.
    result=subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24',
                           'python3',str(test/'driver.py'),str(test/'test.exe')],
                          cwd=test,env=env,capture_output=True,text=True,timeout=120)
    print(result.stdout,end='',flush=True)
    assert result.returncode==0,(result.returncode,result.stderr)
    log=(test/'logfile/render-settings.txt').read_text()
    assert 'uncaught exception:' not in log and 'finished_all!' in log
    assert (test/'settings.json').exists()
    assert not select.select([endpoint],[],[],0)[0], 'Settings-only test attempted a network request'
    print('PASS: positive integer validation, defaults, cancel/save/reopen, independent foreground settings and clean shutdown.')
