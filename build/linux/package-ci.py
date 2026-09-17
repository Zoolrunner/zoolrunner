#!/usr/bin/env python3
"""Stage a self-contained development runtime and verify its ELF architecture."""
import argparse
import json
from pathlib import Path
import re
import shutil
import tarfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('arch', choices=['i686', 'x86_64', 'aarch64'])
p.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
p.add_argument('work', type=Path)
p.add_argument('--toolkit', choices=['gtk2', 'xlib'], default='gtk2')
a = p.parse_args()
work = a.work.resolve()
objname = 'obj-zoolrunner-linux-' + a.arch + '-' + a.app + ('-xlib' if a.toolkit == 'xlib' else '')
obj = work / 'source' / objname
config = (obj / 'config/autoconf.mk').read_text()
toolkit = re.search(r'^MOZ_WIDGET_TOOLKIT\s*=\s*(\S+)', config, re.M).group(1)
if toolkit != a.toolkit:
    raise RuntimeError('Configured toolkit does not match package: ' + toolkit)
stage = work / 'package' / ('zoolrunner-linux-' + a.arch + '-' + a.app + '-' + a.toolkit)
if stage.exists():
    shutil.rmtree(str(stage))
runtime = stage / 'runtime'
shutil.copytree(str(obj / 'dist/bin'), str(runtime), symlinks=False)
# These are build-host utilities, not application runtime dependencies.
for name in ['nsinstall', 'dirver', 'xpidl', 'xpt_link', 'xpt_dump']:
    f = runtime / name
    if f.exists():
        f.unlink()
for name in ['compreg.dat', 'xpti.dat']:
    f = runtime / 'components' / name
    if f.exists():
        f.unlink()
if a.app == 'xulrunner':
    shutil.copytree(str(obj / 'dist/xpi-stage/simple'), str(stage / 'applications/simple'))
elfs = []
for f in runtime.rglob('*'):
    if not f.is_file():
        continue
    with f.open('rb') as stream:
        header = stream.read(20)
    if header[:4] != b'\x7fELF':
        continue
    expected = {'i686': (1, 3), 'x86_64': (2, 62), 'aarch64': (2, 183)}[a.arch]
    if (header[4], int.from_bytes(header[18:20], 'little')) != expected or header[5] != 1:
        raise RuntimeError('Wrong ELF architecture: ' + str(f))
    elfs.append(str(f.relative_to(stage)))
if not elfs:
    raise RuntimeError('No ELF binaries found')
appname = re.search(r'^MOZ_APP_NAME\s*=\s*(\S+)', config, re.M).group(1)
(work / 'logs/package.json').write_text(json.dumps({'arch': a.arch, 'app': a.app, 'toolkit': a.toolkit,
    'appname': appname, 'elfs': elfs}, indent=2) + '\n')
archive = work / 'artifacts' / (stage.name + '.tar.gz')
with tarfile.open(str(archive), 'w:gz', dereference=True) as tar:
    tar.add(str(stage), arcname=stage.name)
print(str(archive))
