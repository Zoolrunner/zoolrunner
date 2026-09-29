#!/usr/bin/env python3
"""Exercise unchanged built ChatZilla assets in a disposable XULRunner application."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--extension', type=Path, required=True,
                        help='Built ChatZilla extension containing chrome.manifest and components')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    source = args.runtime.resolve()
    extension = args.extension.resolve()
    for name in ('chrome.manifest', 'chrome/chatzilla.jar', 'components/chatzilla-service.js'):
        if not (extension / name).is_file():
            parser.error('Missing built extension asset: ' + name)
    with tempfile.TemporaryDirectory(prefix='zool-chatzilla-') as temporary:
        base = Path(temporary)
        runtime, app, profile, home = [base / name for name in ('runtime', 'app', 'profile', 'home')]
        shutil.copytree(source, runtime, symlinks=False)
        for name in ('compreg.dat', 'xpti.dat'):
            (runtime / 'components' / name).unlink(missing_ok=True)
        shutil.copytree(extension, app, symlinks=False)
        profile.mkdir()
        home.mkdir()
        (app / 'application.ini').write_text('''[App]
Vendor=Mozilla
Name=ChatZilla
Version=0.9
BuildID=2006010100
ID={59c81df5-4b7a-477b-912d-4e0fdf64e5f2}
[Gecko]
MinVersion=1.8
MaxVersion=1.8.*
''')
        fixture = app / 'test-chrome'
        fixture.mkdir()
        shutil.copy2(Path(__file__).with_name('standalone-startup.xul'), fixture / 'startup.xul')
        with (app / 'chrome.manifest').open('a') as manifest:
            manifest.write('\ncontent zoolchatzillatest test-chrome/\n')
        prefs = {
            'toolkit.defaultChromeURI': 'chrome://zoolchatzillatest/content/startup.xul',
            'browser.dom.window.dump.enabled': True,
            'extensions.irc.initialURLs': '',
            'extensions.irc.instrumentation.ceip': False,
            'nglayout.debug.disable_xul_cache': True,
            'nglayout.debug.disable_xul_fastload': True,
        }
        (profile / 'user.js').write_text(''.join(
            'user_pref(' + json.dumps(k) + ',' + json.dumps(v) + ');\n' for k, v in prefs.items()))
        env = dict(os.environ, HOME=str(home), LD_LIBRARY_PATH=str(runtime),
                   MOZILLA_FIVE_HOME=str(runtime), MOZ_NO_REMOTE='1')
        executable = runtime / 'xulrunner-bin'
        if not executable.exists():
            executable = runtime / 'xulrunner'
        log = args.report.with_suffix('.log')
        with log.open('w') as output:
            try:
                status = subprocess.run([str(executable), str(app / 'application.ini'),
                                         '-profile', str(profile)], env=env,
                                        stdout=output, stderr=subprocess.STDOUT, timeout=60).returncode
            except subprocess.TimeoutExpired:
                status = 'timeout'
        markers = [line[len('STANDALONE-CHATZILLA '):] for line in log.read_text(errors='replace').splitlines()
                   if line.startswith('STANDALONE-CHATZILLA ')]
        report = {'pass': False, 'exit': status, 'runtime': str(source), 'extension': str(extension)}
        if len(markers) == 1:
            report['result'] = json.loads(markers[0])
            report['pass'] = status == 0 and report['result']['failures'] == 0 and report['result']['checks'] == 12
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report))
        return 0 if report['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
