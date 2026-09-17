#!/usr/bin/env python3
from pathlib import Path
from xml.sax.saxutils import quoteattr
import argparse, json, os, plistlib, shutil, signal, subprocess, tarfile, tempfile, time
parser = argparse.ArgumentParser(description='Run relocated modern macOS package regressions in a desktop session.')
parser.add_argument('arch', choices=['arm64', 'x86_64'])
parser.add_argument('app', choices=['suite', 'browser', 'calendar', 'xulrunner'])
parser.add_argument('--archive', type=Path, required=True)
parser.add_argument('--report-dir', type=Path, required=True)
parser.add_argument('--objdir', type=Path, help='Matching object directory providing JSAPI test headers')
args = parser.parse_args()
root = Path(__file__).resolve().parents[3]
tests = root / 'build/macosx/tests'
report = args.report_dir.resolve()
report.mkdir(parents=True, exist_ok=True)
(report / 'result.txt').unlink(missing_ok=True)
env = dict(os.environ, MOZ_NO_REMOTE='1')
env.pop('DYLD_LIBRARY_PATH', None)
env.pop('DYLD_INSERT_LIBRARIES', None)
archive = args.archive.resolve()

def run(command, name, timeout=120, environment=env):
    r = subprocess.run([str(x) for x in command], env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=timeout)
    (report / (name + '.log')).write_text(r.stdout)
    if r.returncode:
        raise RuntimeError(name + ' failed: ' + r.stdout[-2000:])
    return r.stdout
with tempfile.TemporaryDirectory(prefix='zool-modern-' + args.arch + '-' + args.app + '-') as temp:
    base = Path(temp)
    with tarfile.open(archive) as tar:
        tar.extractall(base)
    stage = next(base.glob('zoolrunner-*'))
    if args.app == 'xulrunner':
        runtime = stage / 'xulrunner'
        exe = runtime / 'xulrunner-bin'
    else:
        bundle = next(stage.glob('*.app'))
        runtime = bundle / 'Contents/MacOS'
        exe = runtime / plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())['CFBundleExecutable']
    shell = runtime / 'xpcshell'
    for name in ['object-reflection', 'object-descriptors', 'json-bind-string', 'array-date', 'library-edge-cases', 'strict-mode', 'legacy-application', 'debugger-lifecycle']:
        out = run([shell, '-f', root / 'js/tests/es5' / (name + '.js')], name)
        assert 'failures=0' in out, (name, out)
    sdk = Path(os.environ.get('ZR_MACOS_SDK', str(Path.home() / 'dev/macos-sdk/MacOSX11.3.sdk')))
    if plistlib.loads((sdk / 'SDKSettings.plist').read_bytes()).get('Version') != '11.3':
        raise RuntimeError('Modern macOS regressions require SDK 11.3')
    includes = (args.objdir or root / ('obj-zoolrunner-macos-' + args.arch + '-' + args.app)) / 'dist/include'
    reg = base / 'regexp-abort'
    run(['xcrun', 'clang', '-arch', args.arch, '-isysroot', sdk, '-DXP_UNIX', '-DJS_THREADSAFE', '-DMOZILLA_1_8_BRANCH', '-I' + str(includes / 'js'), '-I' + str(includes / 'nspr'), root / 'js/tests/es5/TestRegExpAbort.c', '-L' + str(runtime), '-lmozjs', '-o', reg], 'regexp-build')
    assert 'REGEXP-ABORT checks=3 failures=0' in run([reg], 'regexp', environment=dict(env, DYLD_LIBRARY_PATH=str(runtime)))
    if args.app == 'suite':
        out = run(['python3', root / 'editor/composer/tests/run-lifecycle.py', '--runtime', runtime, '--report', report / 'lifecycle-details.log'], 'lifecycle', 150)
        assert 'PLATFORM-LIFECYCLE checks=24 failures=0' in out
    fixture = base / 'fixture'
    fixture.mkdir()
    (fixture / 'early-page.html').write_bytes((tests / 'early-page.html').read_bytes())
    code = (tests / 'early-application.xul').read_text().replace('/tmp/application-result.txt', str(base / 'result.txt')).replace('/tmp/work/early-page.html', str(fixture / 'early-page.html')).replace('Original Mac OS X 10.0 / PowerPC', 'Modern macOS / ' + args.arch)
    (fixture / 'early-application.xul').write_text(code)
    for name in ['window-bootstrap.xul', 'legacy-window-syntax.js']:
        shutil.copy2(root / 'js/tests/es5' / name, fixture / name)
    for name in ['window-editions.xul', 'window-editions.html', 'edition-modern.js']:
        shutil.copy2(root / 'js/tests/es6' / name, fixture / name)
    edition_fixture = fixture / 'window-editions.xul'
    edition_fixture.write_text(edition_fixture.read_text().replace(
        'chrome://es6window/content/window-editions.html',
        (fixture / 'window-editions.html').as_uri()))
    # Calendar intentionally omits data:; use an ordinary file content global.
    (fixture / 'window-content.html').write_text('<html><head><title>Content global</title></head><body>Content global</body></html>')
    window_fixture = fixture / 'window-bootstrap.xul'
    window_fixture.write_text(window_fixture.read_text().replace(
        'data:text/html,&lt;title&gt;Content global&lt;/title&gt;',
        (fixture / 'window-content.html').as_uri()))
    shutil.copy2(tests / 'modern-chatzilla.xul', fixture / 'chatzilla.xul')
    (runtime / 'chrome/zooltest.manifest').write_text('content zooltest ' + fixture.as_uri() + '/\n')
    if args.app == 'calendar':
        shutil.copy2(tests / 'early-calendar-commandline.js', runtime / 'components/zool-test-commandline.js')
    profile = base / 'profile'
    profile.mkdir()
    info = None
    profile_name = base.name

    def profile_shell(source, name):
        script = base / (name + '.js')
        script.write_text(source)
        return run([shell, '-f', script], name)
    if args.app == 'suite':
        source = "var p=Components.classes['@mozilla.org/profile/manager;1'].getService(Components.interfaces.nsIProfile);var old=p.currentProfile;p.createNewProfile(NAME,BASE,null,false);print('PROFILE='+JSON.stringify({original:old,path:p.QueryInterface(Components.interfaces.nsIProfileInternal).getProfileDir(NAME).path}));".replace('NAME', json.dumps(profile_name)).replace('BASE', json.dumps(str(base)))
        out = profile_shell(source, 'profile-create')
        info = json.loads(next((l[8:] for l in out.splitlines() if l.startswith('PROFILE='))))
        profile = Path(info['path'])
        chrome = profile / 'chrome'
        chrome.mkdir(exist_ok=True)
        (chrome / 'chrome.rdf').write_text('<?xml version="1.0"?><RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:c="http://www.mozilla.org/rdf/chrome#"><RDF:Seq RDF:about="urn:mozilla:package:root"><RDF:li RDF:resource="urn:mozilla:package:zooltest"/></RDF:Seq><RDF:Description RDF:about="urn:mozilla:package:zooltest" c:name="zooltest" c:locType="profile" c:baseURL=BASE/></RDF:RDF>'.replace('BASE', quoteattr(fixture.as_uri() + '/')))
    prefs = {'browser.dom.window.dump.enabled': True, 'browser.shell.checkDefaultBrowser': False, 'browser.startup.homepage_override.mstone': 'ignore', 'nglayout.debug.disable_xul_cache': True, 'nglayout.debug.disable_xul_fastload': True, 'zoolrunner.test.application': args.app, 'zoolrunner.test.profile': str(profile), 'toolkit.defaultChromeURI': 'chrome://zooltest/content/early-application.xul'}
    (profile / 'user.js').write_text(''.join(('user_pref(' + json.dumps(k) + ',' + json.dumps(v) + ');\n' for (k, v) in prefs.items())))

    def stop_owned():
        for line in subprocess.check_output(['ps', '-axo', 'pid=,command='], text=True).splitlines():
            (pid, _, command) = line.strip().partition(' ')
            if command.startswith(str(exe) + ' '):
                try:
                    os.kill(int(pid), signal.SIGTERM)
                except ProcessLookupError:
                    pass
    try:
        cases = [('application', 'early-application.xul', 'APPLICATION PASS:'),
                 ('window', 'window-bootstrap.xul', 'WINDOW-BOOTSTRAP checks=17 failures=0'),
                 ('editions', 'window-editions.xul', 'ES6-WINDOW-EDITIONS checks=5 failures=0')]
        if args.app == 'suite':
            cases.append(('chatzilla', 'chatzilla.xul', 'SUITE-CHATZILLA initialized=true'))
        original = code
        for (name, filename, marker) in cases:
            (base / 'result.txt').unlink(missing_ok=True)
            if filename != 'early-application.xul':
                (fixture / 'early-application.xul').write_text((fixture / filename).read_text())
            else:
                (fixture / 'early-application.xul').write_text(original)
            command = [str(exe)]
            if args.app == 'xulrunner':
                command += [str(stage / 'applications/simple/application.ini')]
            command += ['-P', profile_name] if args.app == 'suite' else ['-profile', str(profile)]
            command += ['-zoolrunner-test'] if args.app == 'calendar' else ['-chrome', 'chrome://zooltest/content/early-application.xul']
            path = report / (name + '.log')
            with path.open('w') as log:
                proc = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                deadline = time.monotonic() + 120
                while time.monotonic() < deadline:
                    text = path.read_text(errors='replace')
                    if marker in text:
                        break
                    if 'APPLICATION FAIL:' in text:
                        raise RuntimeError(text[-2000:])
                    time.sleep(1)
                else:
                    raise RuntimeError(name + ' GUI timed out: ' + path.read_text(errors='replace')[-2000:])
            stop_owned()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            time.sleep(1)
    finally:
        stop_owned()
        if info:
            cleanup = "var C=Components.classes,I=Components.interfaces;var r=C['@mozilla.org/registry;1'].createInstance(I.nsIRegistry);r.open(C['@mozilla.org/file/directory_service;1'].getService(I.nsIProperties).get('AppRegF',I.nsIFile));var k=r.getKey(I.nsIRegistry.Common,'Profiles');if(r.getString(k,'CurrentProfile')==NAME){r.setString(k,'CurrentProfile',ORIGINAL);r.flush();}var p=C['@mozilla.org/profile/manager;1'].getService(I.nsIProfile);if(p.profileExists(NAME))p.deleteProfile(NAME,false);".replace('NAME', json.dumps(profile_name)).replace('ORIGINAL', json.dumps(info['original']))
            profile_shell(cleanup, 'profile-cleanup')
(report / 'result.txt').write_text('PASS\n')
print(args.arch + ' ' + args.app + ': shell, regexp and GUI regressions passed', flush=True)
