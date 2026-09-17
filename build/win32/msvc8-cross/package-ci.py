#!/usr/bin/env python3
"""Package a fresh MSVC/Wine application build, retaining classic runtime APIs."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
p.add_argument('objdir', type=Path)
p.add_argument('work', type=Path)
a = p.parse_args()
obj, work = a.objdir.resolve(), a.work.resolve()
runtime = work / 'runtime'
if runtime.exists():
    p.error('Runtime staging directory already exists')
if a.app == 'suite':
    package = work / 'suite-package'
    subprocess.run(['make', '-C', str(obj / 'xpinstall/packager'), 'dist',
                    'MOZ_PKG_DEST=' + str(package), 'MOZ_PKG_APPNAME=zoolrunner'], check=True)
    archive, = package.glob('*.zip')
    with zipfile.ZipFile(archive) as z:
        z.extractall(package)
    shutil.move(str(package / 'zoolrunner'), runtime)
else:
    runtime.mkdir()
    # The old Browser/Calendar installer manifests require a monolithic static
    # executable. These builds use libxul; preserve its complete runtime tree.
    directories = {'chrome', 'components', 'defaults', 'extensions', 'greprefs', 'js',
                   'plugins', 'res', 'dictionaries', 'searchplugins'}
    suffixes = {'.exe', '.dll', '.ini', '.manifest', '.list', '.txt'}
    for source in (obj / 'dist/bin').iterdir():
        if source.is_dir() and source.name in directories:
            shutil.copytree(source, runtime / source.name, symlinks=False)
        elif source.is_file() and (source.suffix.lower() in suffixes or source.name == 'LICENSE'):
            shutil.copy2(source, runtime / source.name)
    if a.app == 'xulrunner':
        samples = work / 'applications'
        samples.mkdir()
        shutil.copytree(obj / 'dist/xpi-stage/simple', samples / 'simple', symlinks=False)
for cache in ['compreg.dat', 'xpti.dat']:
    (runtime / 'components' / cache).unlink(missing_ok=True)
archive = work / 'artifacts' / ('zoolrunner-win32-msvc8-' + a.app + '.zip')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    trees = [(runtime, 'zoolrunner-' + a.app)]
    if a.app == 'xulrunner':
        trees.append((work / 'applications', 'applications'))
    for tree, prefix in trees:
        for source in sorted(tree.rglob('*')):
            if source.is_file():
                z.write(source, prefix + '/' + source.relative_to(tree).as_posix())
(work / 'logs/package.json').write_text(json.dumps({
    'application': a.app, 'archive': archive.name,
    'sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
    'compiler': 'MSVC 2005', 'host': 'Linux/Wine', 'target': 'Windows x86',
}, indent=2) + '\n')
print(archive)
