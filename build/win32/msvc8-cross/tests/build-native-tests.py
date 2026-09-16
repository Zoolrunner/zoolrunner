#!/usr/bin/env python3
"""Build existing C regression programs with the configured MSVC2005 wrappers."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--objdir', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[4]
obj = args.objdir.resolve()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=False)
env = dict(os.environ, MSVC8_USE_PROCESS_HEAP='1', MSVC8_REQUIRE_STATIC_RTL='1')
wrappers = root / 'build/win32/msvc8-cross'
cases = [
    ('expat', 'parser/expat/tests/blocking.c', obj / 'parser/expat/lib/expat_s.lib'),
    ('regexp', 'js/tests/es5/TestRegExpAbort.c', obj / 'dist/lib/js3250.lib'),
    ('embedding', 'js/tests/es5/TestObjectEmbedding.c', obj / 'dist/lib/js3250.lib'),
]
for name, source, library in cases:
    object_file = output / (name + '.obj')
    subprocess.run([
        str(wrappers / 'cl'), '-nologo', '-TC', '-MT', '-c',
        '-DXP_WIN', '-DJS_THREADSAFE', '-DMOZILLA_1_8_BRANCH',
        '-I' + str(obj / 'dist/include/js'), '-I' + str(obj / 'dist/include/nspr'),
        '-I' + str(root / 'parser/expat'), '-I' + str(root / 'parser/expat/lib'),
        str(root / source), '-Fo' + str(object_file),
    ], env=env, check=True)
    subprocess.run([
        str(wrappers / 'link'), '-nologo', '-subsystem:console,4.0',
        '-out:' + str(output / (name + '.exe')), str(object_file), str(library),
    ], env=env, check=True)
