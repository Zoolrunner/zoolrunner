#!/usr/bin/env python3
"""Stage existing Suite regressions for a legacy Windows VM (host-side tool)."""
import argparse
import json
from pathlib import Path
import re
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', type=Path, required=True, help='Unpacked, audited Suite package')
parser.add_argument('--xpcshell', type=Path, help='Matching build xpcshell.exe when omitted from package')
parser.add_argument('--native-tests', type=Path, required=True, help='Matching native C regression binaries')
parser.add_argument('--output', type=Path, required=True, help='New staging directory')
args = parser.parse_args()
root = Path(__file__).resolve().parents[4]
shell = args.xpcshell or args.runtime / 'xpcshell.exe'
if not (args.runtime / 'zoolrunner.exe').is_file() or not shell.is_file():
    parser.error('runtime must contain zoolrunner.exe and xpcshell.exe')
registration = args.runtime / 'chrome/installed-chrome.txt'
if not registration.is_file():
    parser.error('Suite package is missing installed-chrome.txt')
for jar in set(re.findall(r'jar:resource:/+chrome/([^/!]+\.jar)!',
                          registration.read_text())):
    if not (args.runtime / 'chrome' / jar).is_file():
        parser.error('Chrome registration refers to an unshipped archive: ' + jar)
for name in ['expat', 'regexp', 'embedding']:
    if not (args.native_tests / (name + '.exe')).is_file():
        parser.error('native test binary missing: ' + name)
args.output.mkdir(parents=True, exist_ok=False)
payload = args.output / 'payload'
shutil.copytree(args.runtime, payload / 'app', symlinks=False)
if args.xpcshell:
    shutil.copy2(shell, payload / 'app/xpcshell.exe')
for name in ['expat', 'regexp', 'embedding']:
    shutil.copy2(args.native_tests / (name + '.exe'), payload / 'app')
fixtures = payload / 'tests'
fixtures.mkdir()
cases = ['object-reflection', 'object-descriptors', 'json-bind-string', 'array-date',
         'library-edge-cases', 'strict-mode', 'legacy-application', 'debugger-lifecycle']
for name in cases + ['legacy-window-syntax']:
    shutil.copy2(root / 'js/tests/es5' / (name + '.js'), fixtures)
shutil.copy2(root / 'build/macosx/tests/early-page.html', fixtures)
code = (root / 'build/macosx/tests/early-application.xul').read_text()
code = code.replace('/tmp/application-result.txt', 'C:\\\\ZRREG916\\\\application.txt')
code = code.replace('file:///tmp/work/early-page.html', 'file:///C:/ZRREG916/tests/early-page.html')
code = code.replace('Original Mac OS X 10.0 / PowerPC', 'Legacy Windows Suite regression')
(fixtures / 'application.xul').write_text(code)
for source, target in [('js/tests/es5/window-bootstrap.xul', 'window.xul'),
                       ('editor/composer/tests/platform-lifecycle.xul', 'lifecycle.xul'),
                       ('build/macosx/tests/modern-chatzilla.xul', 'chatzilla.xul')]:
    shutil.copy2(root / source, fixtures / target)
logger = (Path(__file__).with_name('file-log.js')).read_text()
for name in ['application', 'window', 'lifecycle', 'chatzilla']:
    p = fixtures / (name + '.xul')
    prelude = logger.replace('LOGFILE', json.dumps('C:\\ZRREG916\\' + name + '.log'))
    prelude += '''\nfunction dump(text) { zrWrite(text); }
// Suite's Windows default-browser prompt uses machine registry settings,
// not the browser.shell preference. Skip only that first-run prompt in the
// disposable test windows, without changing the machine's associations.
var zrWindowWatcher = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Components.interfaces.nsIWindowWatcher);
var zrWindowObserver = {
  observe: function(subject, topic, data) {
    if (topic != "domwindowopened") return;
    var win = subject.QueryInterface(Components.interfaces.nsIDOMWindow);
    win.addEventListener("DOMContentLoaded", function(event) {
      if (event.target == win.document &&
          win.document.documentURI == "chrome://navigator/content/navigator.xul") {
        win.checkForDefaultBrowser = function() {};
      }
    }, true);
  }
};
zrWindowWatcher.registerNotification(zrWindowObserver);
window.addEventListener("unload", function() {
  zrWindowWatcher.unregisterNotification(zrWindowObserver);
}, false);
setTimeout(function() {
  dump("FAIL: GUI regression timed out\\n");
  Components.classes["@mozilla.org/toolkit/app-startup;1"]
    .getService(Components.interfaces.nsIAppStartup).quit(3);
}, 120000);
'''
    p.write_text(p.read_text().replace('<script><![CDATA[', '<script><![CDATA[\n' + prelude, 1))
for name in cases + ['inline']:
    (fixtures / (name + '-log.js')).write_text(
        logger.replace('LOGFILE', json.dumps('C:\\ZRREG916\\' + name + '.log')) +
        '\nfunction print(text) { zrWrite(String(text) + "\\n"); }\n')
shutil.copy2(Path(__file__).with_name('run-suite.js'), payload / 'run.js')
batch = '@echo off\r\nc:\r\ncd \\ZRREG916\r\nif exist report.txt goto exists\r\nset MOZ_NO_REMOTE=1\r\nver > version.txt\r\n'
for name in ['expat', 'regexp', 'embedding']:
    batch += 'app\\' + name + '.exe > ' + name + '.log\r\n'
    batch += 'if errorlevel 1 echo FAIL native exit >> ' + name + '.log\r\n'
batch += 'app\\xpcshell.exe -f run.js > console.txt\r\nnotepad report.txt\r\ngoto end\r\n:exists\r\necho Previous results exist; preserve this folder before another run.\r\n:end\r\n'
(payload / 'run.bat').write_bytes(batch.encode('ascii'))
(args.output / 'setup.bat').write_bytes(b'''@echo off\r
if "%1"=="" goto usage\r
if exist C:\\ZRREG916\\NUL goto exists\r
if exist C:\\ZRREG916\\run.js goto exists\r
xcopy %1\\payload\\*.* C:\\ZRREG916\\ /e /i\r
if errorlevel 1 goto end\r
call C:\\ZRREG916\\run.bat\r
goto end\r
:usage\r
echo Supply the CD drive, for example: D:\\setup.bat D:\r
goto end\r
:exists\r
echo C:\\ZRREG916 already exists. Its files were not changed.\r
:end\r
''')
print('Staged payload at', args.output)
