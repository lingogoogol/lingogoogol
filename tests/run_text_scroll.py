#!/usr/bin/env python3
"""Render and operate real single-line scrollbars without accounts or network."""
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'

driver = r'''
import json
from pathlib import Path
import subprocess
import sys
import time
from PIL import ImageGrab
app = subprocess.Popen(['wine64', sys.argv[1]])
step = 0
while app.poll() is None:
    request = Path('input.json')
    if request.exists():
        try: value = json.loads(request.read_text())
        except json.JSONDecodeError: continue
        if value['step'] > step:
            window = subprocess.check_output(['xdotool','search','--onlyvisible','--name','Text scroll regression'], text=True).splitlines()[0]
            subprocess.run(['xdotool','windowfocus','--sync',window], check=True)
            for command in value['commands']:
                kind, *args = command
                action = {'move':['mousemove','--window',window], 'down':['mousedown','1'],
                          'up':['mouseup','1'], 'click':['click','1'], 'wheel':['click'], 'keydown':['keydown'], 'keyup':['keyup']}[kind]
                subprocess.run(['xdotool',*action,*map(str,args)], check=True)
                time.sleep(.08)
            time.sleep(.15)
            if value['screenshot']:
                geometry = dict(line.split('=',1) for line in subprocess.check_output(
                    ['xdotool','getwindowgeometry','--shell',window], text=True).splitlines())
                x,y = int(geometry['X']),int(geometry['Y'])
                screenshot = ImageGrab.grab().crop((x,y,x+800,y+600))
                screenshot.save(f'/tmp/storage-text-scroll-{value["step"]}.png')
                for top,bottom in ((80,118),(180,204)):
                    def red(box):
                        return sum(r > 100 and r > g + 20 and r > b + 20
                                   for r,g,b in screenshot.crop(box).convert('RGB').getdata())
                    assert red((100,top,300,bottom)) > 10, 'No visible text in viewport.'
                    assert red((0,top,100,bottom)) == 0, 'Text escaped the left clip edge.'
                    assert red((300,top,800,bottom)) == 0, 'Text escaped the right clip edge.'
            step = value['step']
            Path('ack').write_text(str(step))
    time.sleep(.02)
sys.exit(app.wait())
'''

with tempfile.TemporaryDirectory(prefix='storage-scroll-test-') as directory:
    test = Path(directory)
    entry = next(e for e in json.loads((build/'compile_commands.json').read_text())
                 if Path(e['file']) == source/'storage/storage.cpp')
    command = shlex.split(entry['command'])
    command[command.index('-o')+1] = str(test/'test.obj')
    command[command.index('-c')+1] = str(source/'tests/text_scroll.cpp')
    subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja','-t','commands','storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = [arg for arg in shlex.split(link.split(' && ')[1])
               if arg not in ('-mwindows','-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o')+1] = str(test/'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test/'test.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in runtime.glob('*.dll'): shutil.copy2(library, test/library.name)
    for resource in ('font','shader'): shutil.copytree(runtime/resource, test/resource)
    process = subprocess.Popen(['xvfb-run','-a','-s','-screen 0 1024x768x24',sys.executable,'-c',driver,str(test/'test.exe')],
        cwd=test, env={**os.environ,'WINEDEBUG':'-all'}, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, start_new_session=True)
    try:
        stdout,stderr = process.communicate(timeout=45)
    finally:
        if process.poll() is None:
            os.killpg(process.pid,signal.SIGKILL)
            process.wait()
    print(stdout,end='')
    if process.returncode: raise RuntimeError(f'Scroll test failed ({process.returncode}): {stderr}')
    print('PASS: clipped Unicode text, native horizontal wheels, Shift+wheel conversion, drag capture, resize, font reload and input cleanup')
