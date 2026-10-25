#!/usr/bin/env python3
"""Verify a missing Vulkan driver shows an error and exits before reading credentials."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

source = Path(__file__).resolve().parents[1]
project = source.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, default=project / 'build/x64-win32-pe-mingw_Release')
parser.add_argument('--runtime', type=Path, default=project / 'install/x64-win32-pe-mingw_Release')
parser.add_argument('--inside', type=Path, help=argparse.SUPPRESS)
args = parser.parse_args()

if args.inside:
    # Disable host ICD discovery in Wine's Vulkan bridge. No credentials or real
    # queue are supplied: even an unexpected graphics success cannot access Drive.
    test = args.inside.resolve()
    env = {**os.environ, 'WINEDEBUG': '-all',
           'VK_DRIVER_FILES': str(test / 'missing-driver.json'),
           'VK_ICD_FILENAMES': str(test / 'missing-driver.json')}
    if env.get('WINEPREFIX'):
        Path(env['WINEPREFIX']).mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen(['wine64', str(test / 'storage.exe')], cwd=test, env=env,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        deadline = time.monotonic() + 30
        while True:
            windows = subprocess.run(['xdotool', 'search', '--name', '^Storage$'],
                                     capture_output=True, text=True)
            if windows.returncode == 0:
                dialog = windows.stdout.splitlines()[0]
                subprocess.run(['xdotool', 'windowfocus', '--sync', dialog], check=True)
                subprocess.run(['xdotool', 'key', 'Return'], check=True)
                break
            if process.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError('The Vulkan startup error dialog did not appear.')
            time.sleep(.05)
        assert process.wait(timeout=15) == 1, 'Fatal graphics error did not return exit status 1.'
        logs = ''.join(path.read_text(errors='replace') for path in (test / 'logfile').glob('*.txt'))
        assert 'Vulkan' in logs and 'finished_all!' in logs, 'Missing failure reason or orderly shutdown marker.'
        assert not (test / 'root').exists() and not (test / 'blob_queue').exists()
        print('PASS: missing Vulkan driver shows a dialog, logs the cause and exits before cloud initialization')
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=10)
else:
    with tempfile.TemporaryDirectory(prefix='storage-startup-test-') as directory:
        test = Path(directory)
        shutil.copy2(args.build / 'storage/storage.exe', test / 'storage.exe')
        for library in args.runtime.glob('*.dll'):
            shutil.copy2(library, test / library.name)
        for resource in ('font', 'shader'):
            shutil.copytree(args.runtime / resource, test / resource)
        subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                        sys.executable, str(Path(__file__).resolve()), '--inside', str(test)],
                       check=True, timeout=60)
