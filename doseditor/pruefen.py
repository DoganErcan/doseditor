"""Baut und prueft den Editor in einer Visual-Studio-Entwicklerkonsole."""
import argparse
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', action='store_true', help='Zusaetzlich alle vier Projektkonfigurationen bauen')
parser.add_argument('--asan', action='store_true', help='Tests mit AddressSanitizer statt Laufzeitpruefungen bauen')
args = parser.parse_args()

# Doppelte Schreibweisen wie PATH/Path stoeren die MSBuild-Dateiverfolgung.
env = {key.upper(): value for key, value in os.environ.items()}
out = root / 'build' / 'tests'
out.mkdir(parents=True, exist_ok=True)

def run(command, cwd=root):
    subprocess.run([str(value) for value in command], cwd=cwd, env=env, check=True)

if args.build:
    for platform in ('Win32', 'x64'):
        for configuration in ('Debug', 'Release'):
            run([
                'MSBuild.exe', 'doseditor.vcxproj', '/nologo', '/verbosity:minimal',
                f'/p:Configuration={configuration}', f'/p:Platform={platform}',
                '/p:TreatWarningAsError=true',
                '/p:OutDir=' + str(root / 'build' / platform / configuration) + '/',
                '/p:IntDir=' + str(root / 'build' / 'obj' / platform / configuration) + '/',
            ])

name = 'editor_tests_asan' if args.asan else 'editor_tests'
run([
    'cl.exe', '/nologo', '/utf-8', '/W4', '/WX', '/std:c17', '/Od', '/Zi',
    '/fsanitize=address' if args.asan else '/RTC1',
    '/Fo' + str(out / (name + '.obj')), '/Fd' + str(out / (name + '.pdb')),
    '/Fe' + str(out / (name + '.exe')), root / 'tests' / 'editor_tests.c',
    '/link', '/INCREMENTAL:NO',
])
run([out / (name + '.exe')], cwd=out)
