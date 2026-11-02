#!/usr/bin/env python3
"""Exercise Windows checkpoint sharing and large resume without cloud access."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

source = Path(__file__).resolve().parents[1]
fixture_path = source/'tests/run_spaces.py'
fixture = {'__file__': str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],
             str(fixture_path), 'exec'), fixture)

with tempfile.TemporaryDirectory(prefix='storage-checkpoint-') as directory:
    test = Path(directory)
    fixture['compile_source']('storage/google/easy/.cpp', source/'tests/mock_google_easy.cpp', test/'easy.obj')
    fixture['compile_source']('storage/storage.cpp', source/'tests/checkpoint_recovery.cpp', test/'test.obj')
    fixture['link'](test/'test.exe', test/'test.obj', test/'easy.obj')
    for dll in (fixture['runtime']/'bin').glob('*.dll'):
        shutil.copy2(dll, test/dll.name)
    env = {**os.environ, 'WINEDEBUG': '-all', 'WINEPREFIX': str(test/'wine'),
           'STORAGE_GPU_TEST_PORT': '9'}
    try:
        phases = sys.argv[1:] or ('sharing', 'resume', 'scale-256', 'scale-512', 'scale-1024', 'fresh')
        for phase in phases:
            result = subprocess.run(['wine64', str(test/'test.exe'), phase], cwd=test,
                                    env=env, capture_output=True, text=True, timeout=90)
            print(result.stdout, end='', flush=True)
            assert result.returncode == 0, result.stdout + result.stderr
    finally:
        server = '/usr/lib/x86_64-linux-gnu/wine/wineserver'
        subprocess.run([server, '-k'], env=env, capture_output=True)
        subprocess.run([server, '-w'], env=env, capture_output=True)
