#!/usr/bin/env python3
"""Exercise folder downloads against a local mock Drive; no credentials are used."""
import base64
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import time
import urllib.request

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'
entries = json.loads((build / 'compile_commands.json').read_text())
with tempfile.TemporaryDirectory(prefix='storage-folder-test-') as directory:
    test = Path(directory)
    dialog = (source / 'storage/dialog/.cpp').read_text()
    start = dialog.index('dialog_t::dialog_t(bool folder)')
    end = dialog.index('auto dialog_t::ok()', start)
    # Substitute only the modal picker; production download callbacks remain unchanged.
    (test / 'dialog.cpp').write_text(dialog[:start] +
        'dialog_t::dialog_t(bool) { m_result.push_back("output"); m_ok = true; }\n\n' + dialog[end:])
    for original, replacement, output in (
        ('storage/storage.cpp', 'folder_download.cpp', 'test.obj'),
        ('storage/google/easy/.cpp', 'mock_google_easy.cpp', 'mock.obj'),
        ('storage/dialog/.cpp', str(test / 'dialog.cpp'), 'dialog.obj'),
    ):
        entry = next(e for e in entries if Path(e['file']) == source / original)
        command = shlex.split(entry['command'])
        command[command.index('-o') + 1] = str(test / output)
        command[command.index('-c') + 1] = str(source / 'tests' / replacement)
        subprocess.run(command, cwd=build, check=True)
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = [arg for arg in shlex.split(link.split(' && ')[1])
               if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(test / 'test.exe')
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(test / 'test.obj')
    command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(test / 'mock.obj')
    command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    command[command.index('storage/dialog/libstorage.dialog.a')] = str(test / 'dialog.obj')
    command.append('storage/dialog/CMakeFiles/storage.dialog.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)
    for library in runtime.glob('*.dll'):
        shutil.copy2(library, test / library.name)
    for resource in ('font', 'shader'):
        shutil.copytree(runtime / resource, test / resource)
    (test / 'blob_queue').write_text('[]')
    (test / 'output').mkdir()
    fixture = {}

    def blob(name, metadata, content):
        fixture[name] = {'name': json.dumps(metadata), 'description': json.dumps(content)}

    def folder(name, children, next_page='null'):
        blob(name, {'next': next_page}, [{'type': kind, 'id': id, 'name': filename}
             for kind, id, filename in children])

    folder('folder-main', [('file', 'file-main', 'payload.bin'), ('folder', 'folder-sub', 'sub'),
                          ('folder', 'folder-empty', 'empty-dir'), ('file', 'slow-empty', 'empty.bin')], 'folder-page-two')
    folder('folder-page-two', [('file', 'file-page', 'page-two.bin')])
    folder('folder-sub', [('file', 'file-child', 'child.bin')])
    folder('folder-empty', [])
    folder('folder-zeros', [('folder', 'folder-empty', 'child'), ('file', 'slow-empty', 'zero.bin')])
    blob('folder-bad', {'next': 'null'}, [[]])
    blob('slow-empty', {'size': 0, 'next': 'null'}, [])
    for id, text, next_page in [('main', 'ABC', 'file-main-next'), ('main-next', 'DEF', 'null'),
                                ('child', 'nested', 'null'), ('page', 'page two', 'null')]:
        blob('file-' + id, {'size': 6 if id.startswith('main') else len(text), 'next': next_page}, ['chunk-' + id])
        blob('chunk-' + id, {}, ['data-' + id])
        fixture['data-' + id] = {'name': '{}', 'description': base64.urlsafe_b64encode(text.encode()).decode().rstrip('=')}
    (test / 'download-fixture.json').write_text(json.dumps(fixture))
    with (test / 'server.log').open('w') as log:
        server = subprocess.Popen(['python3', str(source / 'tests/mock_drive.py')], cwd=test, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 5
            while not (test / 'server-port').exists():
                if server.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError('Mock Drive did not start.')
                time.sleep(.05)
            port = (test / 'server-port').read_text()
            env = {**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': port}
            if env.get('WINEPREFIX'):
                Path(env['WINEPREFIX']).mkdir(parents=True, exist_ok=True)
            result = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'wine64', str(test / 'test.exe')],
                                    cwd=test, env=env, capture_output=True, text=True, timeout=40)
            print(result.stdout, end='', flush=True)
            if result.returncode:
                raise RuntimeError(f'Folder download test failed ({result.returncode}): {result.stderr}')
            with urllib.request.urlopen(f'http://127.0.0.1:{port}/report', timeout=5) as response:
                report = json.load(response)
            assert not any(event[0] == 'PATCH' for event in report['events']), 'Downloads modified mock cloud data.'
        finally:
            server.terminate()
            server.wait(timeout=10)
