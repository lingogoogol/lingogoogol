#!/usr/bin/env python3
"""Finish transfers with large synthetic checkpoint directories under a real GUI."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
fixture_path = source/'tests/run_spaces.py'
fixture = {'__file__': str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-spaces-')")[0],
             str(fixture_path), 'exec'), fixture)

with tempfile.TemporaryDirectory(prefix='storage-task-cleanup-') as directory:
    test = Path(directory)
    fixture['compile_source']('storage/google/easy/.cpp', source/'tests/mock_google_easy.cpp', test/'easy.obj')
    fixture['compile_source']('storage/storage.cpp', source/'tests/task_cleanup.cpp', test/'test.obj')
    fixture['link'](test/'test.exe', test/'test.obj', test/'easy.obj')
    for dll in (fixture['runtime']/'bin').glob('*.dll'):
        shutil.copy2(dll, test/dll.name)
    for resource in ('font', 'shader'):
        shutil.copytree(fixture['runtime']/resource, test/resource)
    # No accounts or network: exercise only completion, durable index and GUI.
    (test/'settings.json').write_text(json.dumps({'spaces': [], 'worker_count': 64}))
    (test/'task').write_text(json.dumps(['1-1', '1-2']))
    for identifier in ('1-1', '1-2'):
        journal = test/'task.data'/identifier
        journal.mkdir(parents=True)
        (journal/'state').write_text('{}')
        for index in range(30000):
            (journal/f'{index}.response').write_bytes(b'checkpoint')
    env = {**os.environ, 'WINEDEBUG': '-all', 'WINEPREFIX': str(test/'wine'),
           'STORAGE_GPU_TEST_PORT': '1'}
    try:
        result = subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                                 'wine64', str(test/'test.exe')], cwd=test, env=env,
                                capture_output=True, text=True, timeout=240)
        print(result.stdout, end='', flush=True)
        assert result.returncode == 0, result.stdout + result.stderr
        log = (test/'logfile/cleanup.txt').read_text()
        assert 'finished_all!' in log and 'uncaught exception:' not in log
        assert json.loads((test/'task').read_text()) == []
        assert not list((test/'task.data').iterdir()), 'Cleanup did not drain at shutdown'
    finally:
        server = '/usr/lib/x86_64-linux-gnu/wine/wineserver'
        subprocess.run([server, '-k'], env=env, capture_output=True)
        subprocess.run([server, '-w'], env=env, capture_output=True)
