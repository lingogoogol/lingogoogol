"""Real mouse/keyboard driver, launched only by run_directory_operations.py."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from urllib.request import urlopen
from PIL import ImageGrab

if len(sys.argv) > 2 and sys.argv[2] == 'breadcrumbs':
    app = subprocess.Popen(['wine64', sys.argv[1], 'breadcrumbs'])
    step = 0
    try:
        while app.poll() is None:
            path = Path('nav-input.json')
            if path.exists():
                value = json.loads(path.read_text())
                if value['step'] > step:
                    window = subprocess.check_output(['xdotool','search','--onlyvisible','--name',
                        'Directory operation regression'], text=True).splitlines()[0]
                    subprocess.run(['xdotool','windowfocus','--sync',window],check=True)
                    for kind,*args in value['commands']:
                        commands={'move':['mousemove','--window',window], 'wheel':['click'],
                            'down':['mousedown','1'], 'up':['mouseup','1'], 'click':['click','1'],
                            'keydown':['keydown'], 'keyup':['keyup'], 'resize':['windowsize','--sync',window]}
                        subprocess.run(['xdotool',*commands[kind],*map(str,args)],check=True)
                        time.sleep(.08)
                    time.sleep(.15)
                    if value.get('separators') or value.get('dividers'):
                        # Hover tooltips may cover the lines: park the pointer
                        # outside the window (bottom-right of the 1600x1200 screen).
                        subprocess.run(['xdotool','mousemove','1599','1199'],check=True)
                        time.sleep(.4)
                        geometry = dict(line.split('=',1) for line in subprocess.check_output(
                            ['xdotool','getwindowgeometry','--shell',window],text=True).splitlines())
                        x,y,width,height=(int(geometry[key]) for key in ('X','Y','WIDTH','HEIGHT'))
                        image=ImageGrab.grab().crop((x,y,x+width,y+height)).convert('RGB')
                        image.save('/tmp/storage-breadcrumb-separators-' + str(value['step']) + '.png')
                        for box in value['separators']:
                            ink=sum(min(pixel)>180 and max(pixel)-min(pixel)<8 for pixel in image.crop(box).getdata())
                            assert ink>2, 'Breadcrumb > has no visible white glyph pixels: ' + str(box)
                        for point in value['dividers']:
                            pixel = image.getpixel(tuple(point))
                            assert 130 <= min(pixel) <= max(pixel) <= 165 and max(pixel)-min(pixel)<3, \
                                'Column or row divider is not dark gray: ' + str((point, pixel))
                    step = value['step']
                    Path('nav-ack').write_text(str(step))
            time.sleep(.02)
        sys.exit(app.wait())
    finally:
        if app.poll() is None:
            app.kill()
            app.wait()

# A real window manager is required: Wine otherwise reports a maximized
# client while X11 keeps the original surface size and clips the right side.
window_manager = subprocess.Popen(['openbox'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(.4)
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
    # Capture the window itself: with window-manager decorations the reported
    # geometry need not match where the client area is on screen.
    from io import BytesIO
    from PIL import Image
    data = subprocess.run(['import', '-window', window, 'png:-'], check=True, capture_output=True).stdout
    result = Image.open(BytesIO(data)).convert('RGB')
    result.save('/tmp/storage-directory-' + name + '.png')
    return result


def click(x, y):
    run('mousemove', '--window', window, x, y, 'click', '1')
    time.sleep(.25)
# Main-window positions come from gui-ui.json, written by the observed build.
# Dialogs keep lgo::modal_dialog_t's layout at the initial 1536 x 1024 size.
def gui():
    while True:
        try:
            return json.loads(Path('gui-ui.json').read_text())
        except (FileNotFoundError, json.JSONDecodeError):
            time.sleep(.02)
def center(rect):
    return rect['x'] + rect['w'] // 2, rect['y'] + rect['h'] // 2
def tool(label):
    wait_for(lambda: label in gui()['toolbar'], 'Missing toolbar button: ' + label)
    return gui()['toolbar'][label]
def row(name, prefix=False):
    match = lambda value: value['name'].startswith(name) if prefix else value['name'] == name
    wait_for(lambda: any(match(value) for value in gui()['rows']), 'Missing row: ' + name)
    return next(value for value in gui()['rows'] if match(value))
def action(name, label):
    # Click only once the row stops moving: a refresh after the previous
    # operation re-lays out the list, and a stale position hits another row.
    previous, deadline = None, time.monotonic() + 20
    while (current := row(name)) != previous:
        if time.monotonic() > deadline:
            raise RuntimeError('Row did not settle: ' + name)
        previous = current
        time.sleep(.3)
    click(*center(next(value for value in current['actions'] if value['text'] == label)))
def crumb(label):
    wait_for(lambda: any(value['text'] == label for value in gui()['crumbs']), 'Missing breadcrumb: ' + label)
    click(*center(next(value for value in gui()['crumbs'] if value['text'] == label)))


def dialog(lines, fields):
    # lgo::modal_dialog_t layout in the current (maximized) window.
    wait_for(lambda: gui()['dialog'], 'Dialog did not open.')
    height = 152 + 28 * lines + 68 * fields
    return (gui()['window_w'] - 720) // 2, max(0, (gui()['window_h'] - height) // 2), height
def answer(lines, fields, ok=True):
    left, top, height = dialog(lines, fields)
    click(left + (640 if ok else 520), top + height - 32)
CREATE, CONFIRM = (0, 1), (2, 0)  # New folder; paste/delete confirmations
def type_name(name):
    left, top, _ = dialog(*CREATE)
    click(left + 200, top + 52 + 44)
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
    wait_for(lambda: gui()['rows'] and gui()['crumbs'], 'The space listing did not load.')
    paste = tool('貼上')
    paste_left, paste_right, paste_y = paste['x'], paste['x'] + paste['w'] - 1, paste['y'] + 8
    initial = screenshot('initial')
    # Action toolbar buttons retain 96x32 geometry beside the 32px account slot.
    # Empty paste remains
    # gray through hover/press and must not open even an informational dialog.
    for state in ('idle', 'hover', 'press'):
        if state == 'hover':
            run('mousemove', '--window', window, *center(paste))
        elif state == 'press':
            run('mousedown','1')
        time.sleep(.25)
        image = screenshot('paste-disabled-' + state)
        for x in (paste_left, paste_left + 1, paste_right - 1, paste_right):
            assert 80 < image.getpixel((x,paste_y))[0] < 200, 'Disabled paste border is not gray or has the wrong width.'
        ink = [pixel for pixel in image.crop((paste_left+22,paste['y']+4,paste_left+75,paste['y']+28)).getdata() if max(pixel) > 80]
        assert ink and all(max(pixel) < 210 and max(pixel)-min(pixel) < 3 for pixel in ink), 'Disabled paste text changed color.'
    run('mouseup','1')
    time.sleep(.3)
    image = screenshot('paste-disabled-release')
    assert image.getpixel((500,490)) == initial.getpixel((500,490)) and not gui()['dialog'], 'Empty paste opened a dialog.'
    if len(sys.argv) > 2 and sys.argv[2] == 'scroll':
        # A wheel event over the name must reach the containing list; horizontal
        # events subsequently move that name without selecting another row.
        view = lambda: gui()['viewer']
        # Await the rendered list rather than an arbitrary delay: Lavapipe may
        # still be rebuilding thousands of deferred glyphs.
        wait_for(lambda: view() and view()['max'] > 0 and view()['thumb']['h'] > 0, 'Directory scrollbar did not appear.')
        first = gui()['rows'][0]
        run('mousemove', '--window', window, first['rect']['x'] + 100, center(first['rect'])[1])
        time.sleep(.3)
        run('click', '--repeat', '3', '--delay', '80', '5')
        wait_for(lambda: view()['offset'] >= 64, 'Vertical wheel did not update the viewport.')
        wait_for(lambda: not gui()['dialog'], 'Unexpected dialog.')
        time.sleep(.3)
        # The first row fully inside the viewport after scrolling.
        top = view()['rect']['y']
        rows = gui()['rows']
        index = next(i for i, value in enumerate(rows) if value['rect']['y'] >= top)
        target = rows[index]
        identifier = lambda value: value['name'].split('-long-')[0]
        name = target['rect']
        run('mousemove', '--window', window, name['x'] + 100, center(name)[1])
        crop = (name['x'], name['y'], name['x'] + name['w'], name['y'] + name['h'])
        before = screenshot('scroll-vertical').crop(crop)
        run('click', '7')
        wait_for(lambda: screenshot('scroll-horizontal').crop(crop).tobytes() != before.tobytes(),
                 'Horizontal name scrolling did not redraw.')
        action(target['name'], '刪除')
        screenshot('scroll-delete-confirm')
        answer(*CONFIRM)
        wait_for(lambda: identifier(target) not in status(), 'Wheel axes selected the wrong directory row.')
        assert identifier(rows[index - 1]) in status() and identifier(rows[index + 1]) in status()
        # Replacing 39 long-name rows takes many frames, and finishing restores
        # the scroll offset saved when it began: wait for the rebuild to end
        # (empty directory status) before beginning the native drag.
        wait_for(lambda: not any(value['name'] == target['name'] for value in gui()['rows']) and
                 gui()['status'] == '', 'The listing did not finish rebuilding.', 60)
        # Drag the thin vertical thumb down, beyond the list, then release.
        thumb = view()['thumb']
        bottom = view()['rect']['y'] + view()['rect']['h']
        run('mousemove', '--window', window, *center(thumb))
        time.sleep(.3)
        run('mousedown', '1')
        time.sleep(2)
        # Cross the viewport while still inside the native window before
        # leaving it; Wine may coalesce the final out-of-window move.
        run('mousemove', '--window', window, center(thumb)[0], bottom - 4)
        time.sleep(.3)
        run('mousemove', '--window', window, gui()['window_w'] + 14, gui()['window_h'] + 126)
        time.sleep(.3)
        run('mouseup', '1')
        wait_for(lambda: view()['offset'] == view()['max'], 'Dragging did not move the vertical thumb to the bottom.')
        screenshot('scroll-bottom')
        # Pixel scrolling aligns the content bottom with the viewport bottom.
        last = gui()['rows'][-1]
        assert identifier(last) == 'folder-39' and last['rect']['y'] + last['rect']['h'] <= bottom
        action(last['name'], '刪除')
        answer(*CONFIRM)
        wait_for(lambda: 'folder-39' not in status(), 'Dragging did not expose the last directory row.')
        run('key', 'alt+F4')
        app.wait(timeout=10)
        assert app.returncode == 0
        logs = ''.join(path.read_text(errors='replace') for path in Path('logfile').glob('*.txt'))
        assert 'finished_all!' in logs and 'uncaught exception:' not in logs
        print('PASS: production directory wheel axes, horizontal names, thin thumb drag, last-row actions and normal close')
        sys.exit(0)
    click(*center(tool('新增資料夾')))
    screenshot('create')
    type_name('declined')
    answer(*CREATE, ok=False)
    assert len(status()) == 3, 'Cancelled creation changed Drive.'
    click(*center(tool('新增資料夾')))
    type_name('a/b')
    answer(*CREATE)
    assert len(status()) == 3, 'Invalid name changed Drive.'
    screenshot('invalid-name')
    type_name('created')
    answer(*CREATE)
    wait_for(lambda: len(status()) == 4, 'New folder was not created.')
    time.sleep(.6)
    screenshot('created')
    # Row actions are separated by the standard light-blue vertical dividers.
    image = screenshot('actions')
    actions = gui()['rows'][0]['actions']
    for left, right in zip(actions, actions[1:]):
        x = (left['x'] + left['w'] + right['x']) // 2
        span = range(left['y'], left['y'] + left['h'])
        assert sum(b > 240 and 150 < r < 220 and abs(r-g) < 3
                   for r,g,b in (image.getpixel((x,y)) for y in span)) >= len(span) * 3 // 4, 'Missing light-blue action separator.'
    action('file', '剪下')  # Cut records a selection; it must not mutate Drive.
    image = screenshot('paste-enabled')
    assert image.getpixel((paste_left,paste_y))[0] > 240, 'Cut did not enable the standard-size paste button.'
    assert json.loads(status()['file']['name'])['parent'] == 'root'
    click(*center(row('destination')['rect']))  # Browse the destination folder.
    time.sleep(.6)
    click(*center(paste))  # Paste opens a confirmation for the visible folder.
    screenshot('paste-cut')
    answer(*CONFIRM, ok=False)
    assert json.loads(status()['file']['name'])['parent'] == 'root', 'Cancelled paste moved the file.'
    click(*center(paste))
    answer(*CONFIRM)
    wait_for(lambda: json.loads(status()['file']['name'])['parent'] == 'destination', 'Cut/paste did not use the visible folder.')
    time.sleep(.6)
    image = screenshot('paste-empty-after-cut')
    assert 80 < image.getpixel((paste_left,paste_y))[0] < 200, 'Cut paste did not disable the empty clipboard.'
    action('file', '複製')  # Copy the file to the same visible folder.
    click(*center(paste))
    screenshot('paste-copy')
    answer(*CONFIRM)
    wait_for(lambda: len(json.loads(status()['destination']['description'])) == 2, 'Copy/paste did not retain both same-named files.')
    time.sleep(.6)
    screenshot('copy-progress')
    copied = [value['id'] for value in json.loads(status()['destination']['description']) if value['id'] != 'file'][0]
    assert json.loads(status()[copied]['name'])['parent'] == 'destination'
    crumb('測試空間')  # Return to root while retaining the copy selection.
    time.sleep(.6)
    action('destination', '刪除')  # Delete the folder, including both file instances.
    screenshot('delete')
    answer(*CONFIRM, ok=False)
    assert 'destination' in status() and 'file' in status(), 'Cancelled deletion changed Drive.'
    action('destination', '刪除')
    answer(*CONFIRM)
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
    window_manager.kill()
