#!/usr/bin/env python3
"""Run shell and application-window regressions from a relocated Linux package."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('arch', choices=['i686', 'x86_64', 'aarch64'])
p.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
p.add_argument('work', type=Path)
p.add_argument('--toolkit', choices=['gtk2', 'xlib'], default='gtk2')
a = p.parse_args()
work = a.work.resolve()
objname = 'obj-zoolrunner-linux-' + a.arch + '-' + a.app + ('-xlib' if a.toolkit == 'xlib' else '')
root = work / 'source'
logs = work / 'logs'
name = 'zoolrunner-linux-' + a.arch + '-' + a.app + '-' + a.toolkit
metadata = json.loads((logs / 'package.json').read_text())
with tempfile.TemporaryDirectory(prefix='zoolrunner-linux-test-') as tmp:
    base = Path(tmp)
    with tarfile.open(str(work / 'artifacts' / (name + '.tar.gz'))) as archive:
        archive.extractall(str(base))
    stage = base / name
    runtime = stage / 'runtime'
    home = base / 'home'
    home.mkdir()
    # Unix XPCOM uses MOZILLA_FIVE_HOME (or cwd) to locate components.
    # LD_LIBRARY_PATH alone only lets the ELF loader find shared libraries.
    env = dict(os.environ, LD_LIBRARY_PATH=str(runtime), MOZILLA_FIVE_HOME=str(runtime),
               HOME=str(home), MOZ_NO_REMOTE='1')

    def run(command, label, marker=None, expected=0, data=None, timeout=180):
        log = logs / (label + '.log')
        # Keep partial output when an application hangs and hits the timeout.
        with log.open('w') as output:
            r = subprocess.run([str(x) for x in command], env=env, input=data,
                               stdout=output, stderr=subprocess.STDOUT,
                               universal_newlines=True, timeout=timeout)
        output = log.read_text(errors='replace')
        if r.returncode != expected or (marker and marker not in output):
            raise RuntimeError(label + ': exit=' + str(r.returncode) + '\n' + output[-3000:])
        return output

    imports = []
    for relative in metadata['elfs']:
        binary = stage / relative
        result = subprocess.run(['ldd', str(binary)], env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, universal_newlines=True)
        imports.append(relative + '\n' + result.stdout)
        if 'not found' in result.stdout or str(work / 'source') in result.stdout:
            raise RuntimeError('Unresolved or build-tree dependency: ' + relative + '\n' + result.stdout)
    dependencies = '\n'.join(imports)
    (logs / 'dependencies.log').write_text(dependencies)
    if a.toolkit == 'gtk2' and 'libgtk-x11-2.0' not in dependencies:
        raise RuntimeError('GTK2 package does not load GTK2')
    if a.toolkit == 'xlib' and ('libgtk-' in dependencies or 'libgdk-' in dependencies):
        raise RuntimeError('Xlib package unexpectedly depends on GTK/GDK')
    shell = runtime / 'xpcshell'
    for case in ['object-reflection', 'object-descriptors', 'json-bind-string', 'array-date',
                 'library-edge-cases', 'strict-mode', 'legacy-application', 'debugger-lifecycle']:
        run([shell, '-f', root / 'js/tests/es5' / (case + '.js')], case, 'failures=0')
    run([shell, '-e', 'print("INLINE-PASS")'], 'inline', 'INLINE-PASS')
    run([shell, '-f', '-'], 'stdin', 'STDIN-PASS', data='print("STDIN-PASS");\n')
    run([shell, '-e', 'quit(7)'], 'exit-status', expected=7)
    if a.arch == 'aarch64':
        suite = base / 'test262'
        revision = '7da91bceb9ce7613f87db47ddd1292a2dda58b42'
        run(['git', 'init', suite], 'test262-init')
        run(['git', '-C', suite, 'fetch', '--depth=1',
             'https://github.com/tc39/test262.git', revision], 'test262-fetch', timeout=600)
        run(['git', '-C', suite, 'checkout', '--detach', revision], 'test262-checkout')
        report = logs / 'test262.json'
        run(['python3', root / 'js/tests/es5/run-test262.py', '--suite', suite,
             '--shell', shell, '--report', report, '--jobs', '4',
             '--timezone', 'America/Los_Angeles'], 'test262', timeout=1800)
        counts = json.loads(report.read_text())['counts']
        if counts != {'pass': 11540, 'fail': 0, 'timeout': 0, 'crash': 0, 'harness-error': 0}:
            raise RuntimeError('Incomplete pinned Test262 coverage: ' + str(counts))
        if a.app == 'calendar':
            run(['python3', root / 'calendar/test/run-compatibility.py', '--shell', shell,
                 '--report-dir', logs / 'calendar-unit'], 'calendar-unit',
                'CALENDAR-COMPATIBILITY tests=8 failures=0', timeout=600)
    includes = root / objname / 'dist/include'
    for case, source, marker in [('regexp', 'TestRegExpAbort.c', 'checks=3 failures=0'),
                                 ('embedding', 'TestObjectEmbedding.c', 'checks=18 failures=0')]:
        command = ['gcc'] + (['-m32', '-march=i686'] if a.arch == 'i686' else [])
        command += ['-DXP_UNIX', '-DJS_THREADSAFE', '-DMOZILLA_1_8_BRANCH',
                    '-I' + str(includes / 'js'), '-I' + str(includes / 'nspr'),
                    str(root / 'js/tests/es5' / source), '-L' + str(runtime), '-lmozjs', '-o', str(base / case)]
        run(command, case + '-build')
        run([base / case], case, marker)
    if a.arch == 'aarch64':
        xpcom_library = '-lxpcom_core' if (runtime / 'libxpcom_core.so').exists() else '-lxul'
        command = ['g++', '-std=gnu++98', '-fno-rtti', '-fno-exceptions',
                   '-fshort-wchar', '-fno-strict-aliasing', '-O2', '-DXP_UNIX',
                   '-I' + str(includes / 'xpcom'), '-I' + str(includes / 'nspr'),
                   str(root / 'build/linux/TestXPTCallABI.cpp'),
                   '-L' + str(runtime), xpcom_library, '-lplds4', '-lplc4', '-lnspr4',
                   '-o', str(base / 'xptcall-abi')]
        run(command, 'xptcall-abi-build')
        run([base / 'xptcall-abi'], 'xptcall-abi', 'XPTCALL-ABI checks=2000 failures=0')
    expat = base / 'expat'
    command = ['gcc'] + (['-m32', '-march=i686'] if a.arch == 'i686' else [])
    command += ['-DXP_UNIX', '-I' + str(includes / 'nspr'),
                '-I' + str(root / 'parser/expat'), '-I' + str(root / 'parser/expat/lib'),
                str(root / 'parser/expat/tests/blocking.c'),
                str(includes.parent.parent / 'parser/expat/lib/libexpat_s.a'), '-o', str(expat)]
    run(command, 'expat-build')
    run([expat], 'expat', '30 checks passed')
    fixture = base / 'fixture'
    fixture.mkdir()
    tests = root / 'build/macosx/tests'
    for source, dest in [(tests / 'early-page.html', 'early-page.html'),
                         (tests / 'early-contents.rdf', 'contents.rdf'),
                         (root / 'js/tests/es5/legacy-window-syntax.js', 'legacy-window-syntax.js')]:
        shutil.copy2(str(source), str(fixture / dest))
    code = (tests / 'early-application.xul').read_text().replace('/tmp/application-result.txt', str(base / 'result.txt')).replace('/tmp/work/early-page.html', str(fixture / 'early-page.html'))
    (fixture / 'content.html').write_text('<html><title>Content global</title></html>')
    window = (root / 'js/tests/es5/window-bootstrap.xul').read_text().replace(
        'data:text/html,&lt;title&gt;Content global&lt;/title&gt;', (fixture / 'content.html').as_uri())
    (runtime / 'chrome/zooltest.manifest').write_text('content zooltest ' + fixture.as_uri() + '/\n')
    with (runtime / 'chrome/installed-chrome.txt').open('a') as reg:
        reg.write('content,install,url,' + fixture.as_uri() + '/\n')
    if a.app == 'calendar':
        shutil.copy2(str(tests / 'early-calendar-commandline.js'), str(runtime / 'components/zool-test-commandline.js'))
    profile = base / 'profile'
    profile.mkdir()
    if a.app == 'suite':
        helper = base / 'profile.js'
        helper.write_text("var p=Components.classes['@mozilla.org/profile/manager;1'].getService(Components.interfaces.nsIProfile);p.createNewProfile('linux-ci',BASE,null,false);print('PROFILE='+p.QueryInterface(Components.interfaces.nsIProfileInternal).getProfileDir('linux-ci').path);".replace('BASE', json.dumps(str(base))))
        output = run([shell, '-f', helper], 'profile-create')
        profile = Path(next(line[8:] for line in output.splitlines() if line.startswith('PROFILE=')))
    prefs = {'browser.dom.window.dump.enabled': True, 'browser.shell.checkDefaultBrowser': False,
             'browser.startup.homepage_override.mstone': 'ignore', 'nglayout.debug.disable_xul_cache': True,
             'nglayout.debug.disable_xul_fastload': True, 'zoolrunner.test.application': a.app,
             'zoolrunner.test.profile': str(profile)}
    if a.app == 'xulrunner':
        # nsDefaultCLH opens this preference; it has no Browser -chrome handler.
        prefs['toolkit.defaultChromeURI'] = 'chrome://zooltest/content/early-application.xul'
    (profile / 'user.js').write_text(''.join('user_pref(' + json.dumps(k) + ',' + json.dumps(v) + ');\n' for k, v in prefs.items()))
    executable = runtime / (metadata['appname'] + '-bin')
    if not executable.exists():
        executable = runtime / metadata['appname']
    cases = [('application', code, 'APPLICATION PASS:'),
             ('window', window, 'WINDOW-BOOTSTRAP checks=17 failures=0')]
    if a.app == 'suite':
        cases += [('lifecycle', (root / 'editor/composer/tests/platform-lifecycle.xul').read_text(), 'PLATFORM-LIFECYCLE checks=24 failures=0'),
                  ('chatzilla', (tests / 'modern-chatzilla.xul').read_text(), 'SUITE-CHATZILLA initialized=true')]
    for label, content, marker in cases:
        (fixture / 'early-application.xul').write_text(content)
        command = [executable]
        if a.app == 'xulrunner':
            command += [stage / 'applications/simple/application.ini']
        command += ['-P', 'linux-ci'] if a.app == 'suite' else ['-profile', str(profile)]
        if a.app != 'xulrunner':
            command += ['-zoolrunner-test'] if a.app == 'calendar' else ['-chrome', 'chrome://zooltest/content/early-application.xul']
        run(command, label, marker)
    if a.arch == 'aarch64' and a.app == 'calendar':
        shutil.copy2(str(root / 'calendar/test/compatibility-overlay.xul'),
                     str(fixture / 'compatibility-overlay.xul'))
        with (runtime / 'chrome/chrome.manifest').open('a') as manifest:
            manifest.write('\noverlay chrome://calendar/content/calendar.xul '
                           'chrome://zooltest/content/compatibility-overlay.xul\n')
        run([executable, '-profile', profile], 'calendar-window',
            'CALENDAR-WINDOW views=4 failures=0')
(logs / 'runtime-result.txt').write_text('PASS: ' + a.arch + ' ' + a.toolkit + ' ' + a.app + '\n')
print(a.arch + ' ' + a.toolkit + ' ' + a.app + ': packaged runtime checks passed')
