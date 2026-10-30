#!/usr/bin/env python3
"""Exercise general and nested scroll viewports with actual Vulkan and mouse input."""
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
build = source.parent/'build/x64-win32-pe-mingw_Release'
runtime = source.parent/'install/x64-win32-pe-mingw_Release'
driver = r'''
import json
from pathlib import Path
import subprocess
import sys
import time
from PIL import ImageGrab
app = subprocess.Popen(['wine64',sys.argv[1]])
step = 0
while app.poll() is None:
    request = Path('input.json')
    if request.exists():
        try: value = json.loads(request.read_text())
        except json.JSONDecodeError: continue
        if value['step'] > step:
            window = subprocess.check_output(['xdotool','search','--onlyvisible','--name','General scroll regression'],text=True).splitlines()[0]
            subprocess.run(['xdotool','windowfocus','--sync',window],check=True)
            for command in value['commands']:
                kind,*args=command
                actions={'move':['mousemove','--window',window],'down':['mousedown','1'],'up':['mouseup','1'],
                         'click':['click','1'],'wheel':['click'],'keydown':['keydown'],'keyup':['keyup']}
                subprocess.run(['xdotool',*actions[kind],*map(str,args)],check=True)
                time.sleep(.08)
            time.sleep(.15)
            if value['clips'] or value['pixels']:
                geometry = dict(line.split('=',1) for line in subprocess.check_output(['xdotool','getwindowgeometry','--shell',window],text=True).splitlines())
                x,y,w,h=(int(geometry[key]) for key in ('X','Y','WIDTH','HEIGHT'))
                shot=ImageGrab.grab().crop((x,y,x+w,y+h)).convert('RGB')
                shot.save(f'/tmp/storage-general-scroll-{value["step"]}.png')
                for clip in value['clips']:
                    channel=clip['channel']; left,top,right,bottom=clip['box']
                    pixels=shot.load(); visible=0
                    for py in range(h):
                        for px in range(w):
                            color=pixels[px,py]
                            if color[channel] > 100 and all(color[channel] > color[other]+20 for other in range(3) if other!=channel):
                                assert left<=px<right and top<=py<bottom, f'Channel {channel} escaped clip {clip["box"]} at {px,py}.'
                                visible+=1
                    assert visible>5, f'Channel {channel} did not render.'
                for sample in value['pixels']:
                    observed = shot.convert('RGB').getpixel(tuple(sample['point']))
                    assert all(abs(a-b)<=2 for a,b in zip(observed,sample['rgb'])), \
                        'Wrong scrollbar color: ' + str((sample,observed))
            step=value['step']; Path('ack').write_text(str(step))
    time.sleep(.02)
sys.exit(app.wait())
'''
with tempfile.TemporaryDirectory(prefix='storage-general-scroll-') as directory:
    test=Path(directory)
    entry=next(e for e in json.loads((build/'compile_commands.json').read_text()) if Path(e['file'])==source/'storage/storage.cpp')
    command=shlex.split(entry['command'])
    command[command.index('-o')+1]=str(test/'test.obj')
    command[command.index('-c')+1]=str(source/'tests/scroll.cpp')
    subprocess.run(command,cwd=build,check=True)
    link=subprocess.check_output(['ninja','-t','commands','storage.exe'],cwd=build,text=True).splitlines()[-1]
    command=[arg for arg in shlex.split(link.split(' && ')[1]) if arg not in ('-mwindows','-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o')+1]=str(test/'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')]=str(test/'test.obj')
    subprocess.run(command,cwd=build,check=True)
    for library in (runtime / 'bin').glob('*.dll'): shutil.copy2(library,test/library.name)
    for resource in ('font','shader'): shutil.copytree(runtime/resource,test/resource)
    process=subprocess.Popen(['xvfb-run','-a','-s','-screen 0 1200x900x24',sys.executable,'-c',driver,str(test/'test.exe')],
        cwd=test,env={**os.environ,'WINEDEBUG':'-all'},stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,start_new_session=True)
    try: stdout,stderr=process.communicate(timeout=80)
    finally:
        if process.poll() is None: os.killpg(process.pid,signal.SIGKILL);process.wait()
    print(stdout,end='')
    if process.returncode: raise RuntimeError(f'Scroll test failed ({process.returncode}): {stderr}')
    print('PASS: rendered nested viewport intersections contain all colored content; no accounts or network used')
