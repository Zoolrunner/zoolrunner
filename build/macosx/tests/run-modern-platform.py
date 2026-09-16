#!/usr/bin/env python3
from pathlib import Path
import os, subprocess, tempfile, tarfile, plistlib, json
import argparse
parser = argparse.ArgumentParser(description='Check image buffers and Cocoa relaunch against a relocated macOS package.')
parser.add_argument('arch', choices=['arm64', 'x86_64'])
parser.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
parser.add_argument('--archive', type=Path, required=True)
parser.add_argument('--report-dir', type=Path, required=True)
parser.add_argument('--objdir', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[3]
(arch, app) = (args.arch, args.app)
report = args.report_dir.resolve()
report.mkdir(parents=True, exist_ok=True)
(report / 'result.txt').unlink(missing_ok=True)
obj = args.objdir or root / ('obj-zoolrunner-macos-' + arch + '-' + app)
inc = obj / 'dist/include'
sdk = Path(os.environ.get('ZR_MACOS_SDK', str(Path.home() / 'dev/macos-sdk/MacOSX11.3.sdk')))
if plistlib.loads((sdk / 'SDKSettings.plist').read_bytes()).get('Version') != '11.3':
    raise RuntimeError('Modern macOS regressions require SDK 11.3')
env = dict(os.environ)
env.pop('DYLD_LIBRARY_PATH', None)
env.pop('DYLD_INSERT_LIBRARIES', None)

def run(cmd, name, cwd=None, environment=env):
    r = subprocess.run([str(x) for x in cmd], cwd=cwd, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=90)
    (report / (name + '.log')).write_text(r.stdout)
    if r.returncode:
        raise RuntimeError(name + ': ' + r.stdout[-2000:])
    return r.stdout
with tempfile.TemporaryDirectory(prefix='zool-modern-platform-') as tmp:
    base = Path(tmp)
    with tarfile.open(args.archive.resolve()) as t:
        t.extractall(base)
    stage = next(base.glob('zoolrunner-*'))
    runtime = stage / 'xulrunner' if app == 'xulrunner' else next(stage.glob('*.app')) / 'Contents/MacOS'
    libs = [runtime / 'XUL'] if (runtime / 'XUL').exists() else ['-lxpcom', '-lxpcom_core']
    command = ['xcrun', 'clang++', '-arch', arch, '-std=gnu++98', '-isysroot', sdk, '-DMOZILLA_INTERNAL_API', '-fshort-wchar', '-include', obj / 'mozilla-config.h']
    command += ['-I' + str(inc / name) for name in ['xpcom', 'string', 'gfx', 'nspr']]
    command += ['-I' + str(inc), root / 'build/macosx/tests/early-image.cpp', '-L' + str(runtime)] + libs + ['-lplc4', '-lplds4', '-lnspr4', '-o', runtime / 'modern-image']
    run(command, 'image-build')
    out = run([runtime / 'modern-image'], 'image', cwd=runtime)
    assert 'Image frame RGB/alpha top-down rows passed' in out and 'Image RGB/alpha optimization round trip passed' in out
    relaunch = runtime / 'modern-relaunch-bin'
    relaunch_source = base / 'relaunch.mm'
    relaunch_source.write_text((root / 'build/macosx/tests/early-relaunch.mm').read_text().replace(
        '"/tmp/zool-relaunch-result.txt"', json.dumps(str(base / 'relaunch-result.txt'))))
    run(['xcrun', 'clang++', '-arch', arch, '-std=gnu++98', '-isysroot', sdk, '-x', 'objective-c++', relaunch_source, root / 'toolkit/xre/MacLaunchHelper.m', '-I' + str(root / 'toolkit/xre'), '-framework', 'Cocoa', '-o', relaunch], 'relaunch-build')
    for (name, command) in [('absolute', [relaunch]), ('relative', ['./modern-relaunch-bin']), ('path', ['modern-relaunch-bin'])]:
        run(command, 'relaunch-' + name, cwd=runtime, environment=dict(env, PATH=str(runtime) + os.pathsep + env['PATH']))
(report / 'result.txt').write_text('PASS\n')
print(arch + ' ' + app + ': image and three relaunch probes passed', flush=True)
