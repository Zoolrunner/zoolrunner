#!/usr/bin/env python3
"""Exercise packaged Windows applications in the CI container's private Wine prefix."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
p.add_argument('objdir', type=Path)
p.add_argument('work', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[4]
scripts = root / 'build/win32/msvc8-cross'
obj, work = a.objdir.resolve(), a.work.resolve()
logs = work / 'logs'
env = dict(os.environ, MOZ_NO_REMOTE='1')
wine = str(scripts / 'wine-run.sh')

def windows(path):
    return subprocess.check_output([wine, '--winepath', str(path)], env=env,
                                   text=True).strip()

def uri(path):
    return 'file:///' + windows(path).replace('\\', '/')

def run(exe, args, label, marker=None, expected=0, timeout=180, data=None):
    r = subprocess.run([wine, '--', windows(exe)] + args, env=env,
                       input=data, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, errors='replace', timeout=timeout)
    (logs / (label + '.log')).write_text(r.stdout)
    if r.returncode != expected or (marker and marker not in r.stdout):
        raise RuntimeError(label + ': exit=' + str(r.returncode) + '\n' + r.stdout[-3000:])
    return r.stdout

native = work / 'native-tests'
subprocess.run(['python3', str(scripts / 'tests/build-native-tests.py'),
                '--objdir', str(obj), '--output', str(native)], check=True, env=env)
if a.app == 'suite':
    stage = work / 'guest-tests'
    subprocess.run(['python3', str(scripts / 'tests/prepare-suite-regression.py'),
                    '--runtime', str(work / 'runtime'), '--xpcshell', str(obj / 'dist/bin/xpcshell.exe'),
                    '--native-tests', str(native), '--output', str(stage)], check=True)
    target = Path(os.environ['WINEPREFIX']) / 'drive_c/ZRREG916'
    shutil.copytree(stage / 'payload', target)
    (target / 'version.txt').write_text('Linux/Wine CI; not a legacy Windows guest result\r\n')
    for name in ['expat', 'regexp', 'embedding']:
        output = run(target / 'app' / (name + '.exe'), [], 'native-' + name)
        (target / (name + '.log')).write_text(output)
    try:
        run(target / 'app/xpcshell.exe', ['-f', windows(target / 'run.js')], 'suite-driver', timeout=600)
    finally:
        for f in target.iterdir():
            if f.suffix in ('.log', '.txt'):
                shutil.copy2(f, logs / ('suite-' + f.name))
    assert 'RESULT checks=17 failures=0' in (target / 'report.txt').read_text()
else:
    runtime = work / 'runtime-test'
    shutil.copytree(work / 'runtime', runtime)
    shutil.copy2(obj / 'dist/bin/xpcshell.exe', runtime / 'xpcshell.exe')
    shell = runtime / 'xpcshell.exe'
    for name, marker in [('expat', '30 checks passed'), ('regexp', 'checks=3 failures=0'),
                         ('embedding', 'checks=18 failures=0')]:
        shutil.copy2(native / (name + '.exe'), runtime)
        run(runtime / (name + '.exe'), [], name, marker)
    for name in ['object-reflection', 'object-descriptors', 'json-bind-string', 'array-date',
                 'library-edge-cases', 'strict-mode', 'legacy-application', 'debugger-lifecycle']:
        run(shell, ['-f', windows(root / 'js/tests/es5' / (name + '.js'))], name, 'failures=0')
    run(shell, ['-e', 'print("INLINE-PASS")'], 'inline', 'INLINE-PASS')
    run(shell, ['-f', '-'], 'stdin', 'STDIN-PASS', data='print("STDIN-PASS");\n')
    run(shell, ['-e', 'quit(7)'], 'exit-status', expected=7)
    fixture = work / 'fixture'
    fixture.mkdir()
    tests = root / 'build/macosx/tests'
    shutil.copy2(tests / 'early-page.html', fixture)
    shutil.copy2(root / 'js/tests/es5/legacy-window-syntax.js', fixture)
    (fixture / 'window-content.html').write_text('<html><title>Content global</title></html>')
    code = (tests / 'early-application.xul').read_text()
    code = code.replace('/tmp/application-result.txt', windows(work / 'application-result.txt').replace('\\', '\\\\'))
    code = code.replace('file:///tmp/work/early-page.html', uri(fixture / 'early-page.html'))
    window = (root / 'js/tests/es5/window-bootstrap.xul').read_text().replace(
        'data:text/html,&lt;title&gt;Content global&lt;/title&gt;', uri(fixture / 'window-content.html'))
    (runtime / 'chrome/zooltest.manifest').write_text('content zooltest ' + uri(fixture) + '/\n')
    if a.app == 'calendar':
        shutil.copy2(tests / 'early-calendar-commandline.js', runtime / 'components/zool-test-commandline.js')
    profile = work / 'profile'
    profile.mkdir()
    prefs = {'browser.dom.window.dump.enabled': True, 'browser.shell.checkDefaultBrowser': False,
             'browser.startup.homepage_override.mstone': 'ignore', 'nglayout.debug.disable_xul_cache': True,
             'nglayout.debug.disable_xul_fastload': True, 'zoolrunner.test.application': a.app,
             'zoolrunner.test.profile': windows(profile)}
    (profile / 'user.js').write_text(''.join('user_pref(' + json.dumps(k) + ',' + json.dumps(v) + ');\n'
                                           for k, v in prefs.items()))
    appname = re.search(r'^MOZ_APP_NAME\s*=\s*(\S+)', (obj / 'config/autoconf.mk').read_text(), re.M).group(1)
    for label, source, marker in [('application', code, 'APPLICATION PASS:'),
                                   ('window', window, 'WINDOW-BOOTSTRAP checks=17 failures=0')]:
        result = logs / (label + '-result.log')
        logger = (scripts / 'tests/file-log.js').read_text().replace('LOGFILE', json.dumps(windows(result)))
        logger += '\nfunction dump(text) { zrWrite(text); }\n'
        (fixture / 'early-application.xul').write_text(source.replace('<script><![CDATA[', '<script><![CDATA[\n' + logger, 1))
        args = []
        if a.app == 'xulrunner':
            args.append(windows(work / 'applications/simple/application.ini'))
        args += ['-profile', windows(profile)]
        args += ['-zoolrunner-test'] if a.app == 'calendar' else ['-chrome', 'chrome://zooltest/content/early-application.xul']
        run(runtime / (appname + '.exe'), args, label)
        output = result.read_text()
        if marker not in output or 'FAIL' in output:
            raise RuntimeError(label + ': ' + output)
(logs / 'runtime-result.txt').write_text('PASS: ' + a.app + ' on Linux/Wine\n')
print(a.app + ': packaged runtime checks passed')
