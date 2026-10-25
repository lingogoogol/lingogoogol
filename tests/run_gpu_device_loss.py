#!/usr/bin/env python3
"""Run GPU recovery and transfer regression tests in Wine with a local mock Drive."""
import argparse
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
project = source.parent
gpu_modes = ('rect-threads', 'no-depth-format', 'no-present-modes', 'no-device-features', 'image-memory-properties',
             'acquire-suboptimal', 'present-suboptimal', 'fatal-submit', 'fatal-remove', 'fatal-unknown', 'invalid-shader', 'device-fallback', 'capabilities', 'fence-timeout', 'acquire-timeout', 'acquire-not-ready', 'acquire-outdated',
                 'surface-lost', 'present-outdated', 'present-surface-lost', 'depth-fallback', 'no-surface-formats',
                 'out-of-memory', 'logging', 'logging-crash', 'gui-names', 'names', 'flush', 'fence', 'acquire', 'submit', 'present', 'map', 'buffer', 'glyph', 'native',
                 'retry', 'fallback', 'repeated', 'shutdown', 'unwind', 'transfer-shutdown')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, default=project / 'build/x64-win32-pe-mingw_Release')
parser.add_argument('--runtime', type=Path, default=project / 'install/x64-win32-pe-mingw_Release')
parser.add_argument('--mode', action='append', choices=(*gpu_modes, 'transfer'), help='Run only the selected mode; use transfer for the mock Drive test.')
args = parser.parse_args()
build = args.build.resolve()
entries = json.loads((build / 'compile_commands.json').read_text())


def compile_test(original, replacement, output):
    entry = next(e for e in entries if Path(e['file']) == source / original)
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(output)
    command[command.index('-c') + 1] = str(source / 'tests' / replacement)
    subprocess.run(command, cwd=build, check=True)


def link_test(test, main, executable, mock=None):
    link = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = [arg for arg in shlex.split(link.split(' && ')[1])
               if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(test / executable)
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(main)
    if mock:
        command[command.index('storage/google/easy/libstorage.google.easy.a')] = str(mock)
        command.append('storage/google/easy/CMakeFiles/storage.google.easy.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)


def run_test(executable, test, env, *arguments, expected_code=0):
    result = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                             'wine64', str(executable), *arguments], cwd=test, env=env,
                            capture_output=True, text=True, timeout=60)
    print(result.stdout, end='', flush=True)
    if result.returncode != expected_code:
        raise RuntimeError(f'{executable.name} {arguments} failed ({result.returncode}): {result.stderr}')


with tempfile.TemporaryDirectory(prefix='storage-gpu-test-') as directory:
    test = Path(directory)
    compile_test('storage/storage.cpp', 'gpu_device_loss.cpp', test / 'test.obj')
    link_test(test, test / 'test.obj', 'test.exe')
    compile_test('storage/storage.cpp', 'gpu_transfer_recovery.cpp', test / 'transfer.obj')
    compile_test('storage/google/easy/.cpp', 'mock_google_easy.cpp', test / 'mock.obj')
    link_test(test, test / 'transfer.obj', 'transfer.exe', test / 'mock.obj')
    # Copy only public GUI resources and libraries. No credentials, root or real blob queue.
    for library in args.runtime.glob('*.dll'):
        shutil.copy2(library, test / library.name)
    for resource in ('font', 'shader'):
        shutil.copytree(args.runtime / resource, test / resource)
    env = {**os.environ, 'WINEDEBUG': '-all'}
    if env.get('WINEPREFIX'):
        Path(env['WINEPREFIX']).mkdir(parents=True, exist_ok=True)
    for mode in gpu_modes:
        if not args.mode or mode in args.mode:
            if mode == 'logging-crash':
                run_test(test / 'test.exe', test, env, mode, expected_code=86)
                assert (test / 'crash-log.txt').read_bytes() == b'first completed message\nlast completed message\n'
                print('PASS: flushed log messages survive abrupt exit without destructors or closing the log handle', flush=True)
            else:
                run_test(test / 'test.exe', test, env, mode)

    if args.mode and 'transfer' not in args.mode:
        raise SystemExit(0)

    inputs = test / 'input'
    (inputs / 'tree/sub').mkdir(parents=True)
    (inputs / 'tree/empty-folder').mkdir()
    (inputs / 'empty.bin').write_bytes(b'')
    (inputs / 'tree/empty.bin').write_bytes(b'')
    (inputs / 'tree/sub/payload.bin').write_bytes(bytes(range(256)) * 8000)
    (inputs / 'another.bin').write_bytes(b'another payload')
    (inputs / 'visible.bin').write_bytes(b'visible data')
    (test / 'blob_queue').write_text(json.dumps([f'blob-{i}' for i in range(21000)]))
    with (test / 'mock-server.log').open('w') as output:
        server = subprocess.Popen(['python3', str(source / 'tests/mock_drive.py')], cwd=test,
                                  stdout=output, stderr=output)
        try:
            deadline = time.monotonic() + 5
            while not (test / 'server-port').exists():
                if server.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError('Mock Drive did not start.')
                time.sleep(.05)
            port = (test / 'server-port').read_text()
            run_test(test / 'transfer.exe', test, {**env, 'STORAGE_GPU_TEST_PORT': port})
            with urllib.request.urlopen(f'http://127.0.0.1:{port}/report', timeout=5) as response:
                report = json.load(response)
            assert not report['violations'], report['violations']
            assert len(report['root']) == len({item['name'] for item in report['root']}) == 4
            assert report['fail_content_consumed'] and report['fail_publish_consumed']
            print('PASS: mock Drive verified complete publication and no duplicate entries', flush=True)
        finally:
            server.terminate()
            server.wait(timeout=10)
