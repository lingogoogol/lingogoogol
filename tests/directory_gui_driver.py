"""Real mouse/keyboard driver, launched only by run_directory_operations.py."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from urllib.request import urlopen
from PIL import ImageGrab

app = subprocess.Popen(['wine64', sys.argv[1]])
window = None


def run(*args):
    return subprocess.run(['xdotool', *map(str, args)], check=True, capture_output=True, text=True)


def wait_for(predicate, message, timeout=20):
    deadline = time.monotonic() + timeout
    while not predicate():
        if app.poll() is not None or time.monotonic() > deadline:
            raise RuntimeError(message)
        time.sleep(.05)


def status():
    with urlopen('http://127.0.0.1:' + os.environ['STORAGE_GPU_TEST_PORT'] + '/fixture/status') as response:
        return json.load(response)


def screenshot(name):
    geometry = dict(line.split('=', 1) for line in run('getwindowgeometry', '--shell', window).stdout.splitlines())
    x, y, width, height = (int(geometry[key]) for key in ('X', 'Y', 'WIDTH', 'HEIGHT'))
    result = ImageGrab.grab().crop((x, y, x + width, y + height)).convert('RGB')
    result.save('/tmp/storage-directory-' + name + '.png')
    return result


def click(x, y):
    run('mousemove', '--window', window, x, y, 'click', '1')
    time.sleep(.25)


def type_name(name):
    click(500, 490)
    run('key', 'ctrl+a')
    run('type', '--clearmodifiers', name)


def glow(name, x, y):
    run('mousemove', '--window', window, x, y)
    time.sleep(.25)
    image = screenshot(name)
    center = image.getpixel((x, y))[0]
    middle = image.getpixel((x + 36, y))[0]
    outside = image.getpixel((x + 80, y))[0]
    # The swapchain encodes linear shader output as sRGB.
    assert 45 <= center <= 76 and center > middle > outside + 2, (name, center, middle, outside)
    assert image.getpixel((x + 50, y))[0] > outside, 'Highlight radius shrank.'
    for dx, dy in ((-36, 0), (0, 36), (0, -36)):
        assert abs(image.getpixel((x + dx, y + dy))[0] - middle) <= 2, 'Highlight is not circular.'
    return image


try:
    def find_window():
        global window
        result = subprocess.run(['xdotool', 'search', '--onlyvisible', '--name', 'wWinMain.engine'], capture_output=True, text=True)
        if result.returncode:
            return False
        window = result.stdout.splitlines()[0]
        return True
    wait_for(find_window, 'Window did not appear.', 30)
    run('windowfocus', '--sync', window)
    time.sleep(1)
    screenshot('initial')
    # All toolbar buttons have the same 96x32 geometry. Empty paste remains
    # gray through hover/press and must not open even an informational dialog.
    for state in ('idle', 'hover', 'press'):
        if state == 'hover':
            run('mousemove', '--window', window, 856,24)
        elif state == 'press':
            run('mousedown','1')
        time.sleep(.25)
        image = screenshot('paste-disabled-' + state)
        for x in (808, 809, 902, 903):
            assert 80 < image.getpixel((x,16))[0] < 200, 'Disabled paste border is not gray or has the wrong width.'
        ink = [pixel for pixel in image.crop((830,12,883,36)).getdata() if max(pixel) > 80]
        assert ink and all(max(pixel) < 210 and max(pixel)-min(pixel) < 3 for pixel in ink), 'Disabled paste text changed color.'
    run('mouseup','1')
    time.sleep(.3)
    image = screenshot('paste-disabled-release')
    assert image.getpixel((500,490)) == (0,0,0), 'Empty paste opened a dialog.'
    if len(sys.argv) > 2 and sys.argv[2] == 'scroll':
        # A wheel event over the name must reach the containing list; horizontal
        # events subsequently move that name without selecting another row.
        def thumb_top():
            image = screenshot('scroll-sync')
            return next((y for y in range(58, 1016) if image.getpixel((1526, y))[0] > 180), None)
        # Await rendered state rather than assuming that Lavapipe has finished
        # rebuilding thousands of deferred glyphs after an arbitrary delay.
        wait_for(lambda: thumb_top() == 58, 'Directory scrollbar did not appear.')
        run('mousemove', '--window', window, 400, 70)
        time.sleep(.3)
        run('click', '--repeat', '3', '--delay', '80', '5')
        wait_for(lambda: (thumb_top() or 0) >= 80, 'Vertical wheel did not update the viewport.')
        before = screenshot('scroll-vertical').crop((285,60,1500,94))
        run('click', '7')
        wait_for(lambda: screenshot('scroll-horizontal').crop((285,60,1500,94)).tobytes() != before.tobytes(),
                 'Horizontal name scrolling did not redraw.')
        click(456, 96)
        screenshot('scroll-delete-confirm')
        click(1056, 584)
        wait_for(lambda: 'folder-01' not in status(), 'Wheel axes selected the wrong directory row.')
        assert 'folder-00' in status() and 'folder-02' in status()
        # Replacing 39 long-name rows also rebuilds their deferred glyphs.
        # Let the mocked refresh finish before beginning the native drag.
        time.sleep(2)
        # Drag the thin vertical thumb down, beyond the list, then release.
        run('mousemove', '--window', window, 1526, 90)
        time.sleep(.3)
        run('mousedown', '1')
        time.sleep(2)
        # Cross the viewport while still inside the native window before
        # leaving it; Wine may coalesce the final out-of-window move.
        run('mousemove', '--window', window, 1526, 1004)
        time.sleep(.3)
        run('mousemove', '--window', window, 1550, 1150)
        time.sleep(.3)
        run('mouseup', '1')
        wait_for(lambda: (thumb_top() or 0) > 700, 'Dragging did not move the vertical thumb to the bottom.')
        screenshot('scroll-bottom')
        # Pixel scrolling aligns the content bottom with the viewport bottom.
        click(456, 994)
        click(1056, 584)
        wait_for(lambda: 'folder-39' not in status(), 'Dragging did not expose the last directory row.')
        run('key', 'alt+F4')
        app.wait(timeout=10)
        assert app.returncode == 0
        logs = ''.join(path.read_text(errors='replace') for path in Path('logfile').glob('*.txt'))
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
        print('PASS: production directory wheel axes, horizontal names, thin thumb drag, last-row actions and normal close')
        sys.exit(0)
    click(1168, 24)
    screenshot('create')
    type_name('declined')
    click(936, 590)
    assert len(status()) == 3, 'Cancelled creation changed Drive.'
    click(1168, 24)
    type_name('a/b')
    click(1056, 590)
    assert len(status()) == 3, 'Invalid name changed Drive.'
    screenshot('invalid-name')
    type_name('created')
    click(1056, 590)
    wait_for(lambda: len(status()) == 4, 'New folder was not created.')
    time.sleep(.6)
    screenshot('created')
    # Download | Cut | Copy | Delete use the standard vertical dividers.
    image = screenshot('actions')
    for x in (330, 380, 430):
        assert sum(b > 240 and 150 < r < 220 and abs(r-g) < 3
                   for r,g,b in (image.getpixel((x,y)) for y in range(98,125))) >= 18, 'Missing light-blue action separator.'
    click(356, 110)  # Cut records a selection; it must not mutate Drive.
    image = screenshot('paste-enabled')
    assert image.getpixel((808,16))[0] > 240, 'Cut did not enable the standard-size paste button.'
    assert json.loads(status()['file']['name'])['parent'] == 'root'
    click(450, 152)  # Browse the destination folder.
    time.sleep(.6)
    click(880, 24)  # Paste opens a confirmation for the visible folder.
    screenshot('paste-cut')
    click(936, 584)
    assert json.loads(status()['file']['name'])['parent'] == 'root', 'Cancelled paste moved the file.'
    click(880, 24)
    click(1056, 584)
    wait_for(lambda: json.loads(status()['file']['name'])['parent'] == 'destination', 'Cut/paste did not use the visible folder.')
    time.sleep(.6)
    image = screenshot('paste-empty-after-cut')
    assert 80 < image.getpixel((808,16))[0] < 200, 'Cut paste did not disable the empty clipboard.'
    click(406, 151)  # Copy the file to the same visible folder.
    click(880, 24)
    screenshot('paste-copy')
    click(1056, 584)
    wait_for(lambda: len(json.loads(status()['destination']['description'])) == 2, 'Copy/paste did not retain both same-named files.')
    time.sleep(.6)
    screenshot('copy-progress')
    copied = [value['id'] for value in json.loads(status()['destination']['description']) if value['id'] != 'file'][0]
    assert json.loads(status()[copied]['name'])['parent'] == 'destination'
    click(340, 70)  # Return to root while retaining the copy selection.
    time.sleep(.6)
    click(456, 110)  # Delete the first folder, including both file instances.
    screenshot('delete')
    click(936, 584)
    assert 'destination' in status() and 'file' in status(), 'Cancelled deletion changed Drive.'
    click(456, 110)
    click(1056, 584)
    wait_for(lambda: 'destination' not in status() and 'file' not in status() and copied not in status(), 'Recursive deletion did not finish.')
    glow('glow', 1000, 900)
    run('windowsize', '--sync', window, 800, 600)
    time.sleep(.5)
    glow('glow-small', 600, 450)
    run('mousemove', 1500, 1100)
    time.sleep(.25)
    image = screenshot('glow-outside')
    assert image.getpixel((600, 450)) == image.getpixel((680, 450)), 'Highlight remained after mouse leave.'
    run('key', 'alt+F4')
    app.wait(timeout=10)
    assert app.returncode == 0
    logs = ''.join(path.read_text(errors='replace') for path in Path('logfile').glob('*.txt'))
    assert 'finished_all!' in logs and 'uncaught exception:' not in logs
    print('PASS: real GUI create validation/cancel, cut/copy/paste into the visible folder, delete confirmation/cancel, standard action separators, circular shader falloff, resize, mouse leave and normal close')
finally:
    if app.poll() is None:
        app.kill()
        app.wait()
