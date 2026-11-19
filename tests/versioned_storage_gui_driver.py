#!/usr/bin/env python3
"""Deliver native mouse/keyboard input requested by the offline storage regression."""
import json
from pathlib import Path
import subprocess
import sys
import time

app = subprocess.Popen(['wine64', str(Path(sys.argv[1]).resolve()), *sys.argv[2:]])
step = 0
try:
    while app.poll() is None:
        if 'interrupt-upgrade' in sys.argv and Path('crash-now').exists():
            app.kill()
            app.wait()
            sys.exit(0)
        path = Path('ui-step.json')
        if path.exists():
            value = json.loads(path.read_text())
            if value['step'] > step:
                window = subprocess.check_output(
                    ['xdotool', 'search', '--onlyvisible', '--name', 'Versioned storage regression'], text=True).splitlines()[0]
                subprocess.run(['xdotool', 'windowfocus', '--sync', window], check=True)
                for kind, *args in value['commands']:
                    commands = {'move': ['mousemove', '--window', window, *map(str, args)], 'click': ['mousemove', '--window', window, *map(str, args), 'click', '1'],
                                'key': ['key', *args], 'type': ['type', '--clearmodifiers', *args]}
                    if kind == 'capture':
                        from PIL import ImageGrab
                        name, x, y, width, height, *extra = args
                        geometry = dict(line.split('=', 1) for line in subprocess.check_output(
                            ['xdotool','getwindowgeometry','--shell',window],text=True).splitlines())
                        origin_x, origin_y = int(geometry['X']), int(geometry['Y'])
                        crop=ImageGrab.grab().crop((origin_x+x,origin_y+y,origin_x+x+width,origin_y+y+height)).convert('RGB')
                        crop.save('/tmp/storage-tooltip-'+name+'.png')
                        assert sum(min(pixel)>180 for pixel in crop.get_flattened_data())>10, 'Capture has no visible text'
                        if extra:
                            for px, py in extra[0]:
                                red, green, blue = crop.getpixel((px-x,py-y))
                                assert blue>red+30 and abs(red-green)<5 and red>100, 'Navigation separator is not visibly blue'
                        continue
                    subprocess.run(['xdotool', *commands[kind]], check=True)
                    time.sleep(.12)
                step = value['step']
                Path('ui-ack').write_text(str(step))
        time.sleep(.02)
    sys.exit(app.wait())
finally:
    if app.poll() is None:
        app.kill()
        app.wait()
