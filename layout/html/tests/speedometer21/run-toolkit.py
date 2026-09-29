#!/usr/bin/env python3
"""Run content probes in disposable Browser, Calendar or XULRunner copies."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--application', choices=['browser', 'calendar', 'xulrunner'], required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    fixture_input = parser.add_mutually_exclusive_group(required=True)
    fixture_input.add_argument('--url')
    fixture_input.add_argument('--chrome-probe', type=Path,
                               help='Privileged XUL fixture using the same result marker')
    parser.add_argument('--restart-url', action='append', default=[])
    parser.add_argument('--mode', choices=['probe'], default='probe')
    parser.add_argument('--content-edition', choices=['es5', 'es2015'], default='es2015')
    parser.add_argument('--timeout', type=int, default=60)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if args.chrome_probe and args.restart_url:
        parser.error('--restart-url requires --url')
    source_runtime = args.runtime.resolve()
    here = Path(__file__).resolve().parent
    root = here.parents[3]
    args.report.parent.mkdir(parents=True, exist_ok=True)
    reports = []
    with tempfile.TemporaryDirectory(prefix='zool-content-' + args.application + '-') as temporary:
        base = Path(temporary)
        runtime = base / 'runtime'
        shutil.copytree(source_runtime, runtime, symlinks=False)
        for name in ('compreg.dat', 'xpti.dat'):
            (runtime / 'components' / name).unlink(missing_ok=True)
        home, profile, fixture = base / 'home', base / 'profile', base / 'fixture'
        for directory in (home, profile, fixture):
            directory.mkdir()
        shutil.copy2(args.chrome_probe or here / 'runner.xul', fixture / 'early-application.xul')
        (fixture / 'contents.rdf').write_text('''<?xml version="1.0"?>
<RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:c="http://www.mozilla.org/rdf/chrome#">
<RDF:Seq RDF:about="urn:mozilla:package:root"><RDF:li RDF:resource="urn:mozilla:package:zooltest"/></RDF:Seq>
<RDF:Description RDF:about="urn:mozilla:package:zooltest" c:name="zooltest"/>
</RDF:RDF>
''')
        (runtime / 'chrome/zooltest.manifest').write_text('content zooltest ' + fixture.as_uri() + '/\n')
        with (runtime / 'chrome/installed-chrome.txt').open('a') as registration:
            registration.write('content,install,url,' + fixture.as_uri() + '/\n')
        if args.application == 'calendar':
            shutil.copy2(root / 'build/macosx/tests/early-calendar-commandline.js',
                         runtime / 'components/zool-test-commandline.js')
        names = {'browser': 'zrbrowser', 'calendar': 'sunbird', 'xulrunner': 'xulrunner'}
        executable = runtime / (names[args.application] + '-bin')
        if not executable.exists():
            executable = runtime / names[args.application]
        command = [str(executable)]
        if args.application == 'xulrunner':
            application = base / 'application'
            application.mkdir()
            shutil.copy2(root / 'js/tests/es5/window-app/application.ini', application / 'application.ini')
            command.append(str(application / 'application.ini'))
        command += ['-profile', str(profile)]
        if args.application == 'calendar':
            command.append('-zoolrunner-test')
        elif args.application == 'browser':
            command += ['-chrome', 'chrome://zooltest/content/early-application.xul']
        environment = dict(os.environ, HOME=str(home), LD_LIBRARY_PATH=str(runtime),
                           MOZILLA_FIVE_HOME=str(runtime), MOZ_NO_REMOTE='1')
        preferences = {
            'browser.dom.window.dump.enabled': True,
            'browser.shell.checkDefaultBrowser': False,
            'browser.startup.homepage_override.mstone': 'ignore',
            'nglayout.debug.disable_xul_cache': True,
            'nglayout.debug.disable_xul_fastload': True,
            'javascript.options.content.es2015': args.content_edition == 'es2015',
            'zoolrunner.speedometer.mode': 'probe',
            'zoolrunner.speedometer.debugErrors': '',
            'zoolrunner.speedometer.timeout': args.timeout,
            'zoolrunner.test.application': args.application,
            'toolkit.defaultChromeURI': 'chrome://zooltest/content/early-application.xul',
        }
        urls = [args.url or args.chrome_probe.resolve().as_uri()] + args.restart_url
        for index, url in enumerate(urls):
            preferences['zoolrunner.speedometer.url'] = url
            (profile / 'user.js').write_text(''.join(
                'user_pref(' + json.dumps(key) + ',' + json.dumps(value) + ');\n'
                for key, value in preferences.items()))
            log = args.report.with_suffix('.log') if index == 0 else args.report.with_name(
                args.report.stem + '.restart-' + str(index) + '.log')
            with log.open('w') as output:
                try:
                    result = subprocess.run(command, env=environment, stdout=output,
                                            stderr=subprocess.STDOUT, timeout=args.timeout + 60)
                    status = result.returncode
                except subprocess.TimeoutExpired:
                    status = 'timeout'
            markers = [line[len('SPEEDOMETER-RESULT '):] for line in log.read_text(errors='replace').splitlines()
                       if line.startswith('SPEEDOMETER-RESULT ')]
            report = {'pass': False, 'exit': status, 'application': args.application,
                      'runtime': str(source_runtime), 'url': url,
                      'mode': 'chrome-probe' if args.chrome_probe else 'probe',
                      'contentEdition': args.content_edition}
            if len(markers) == 1:
                try:
                    report['result'] = json.loads(markers[0])
                    report['pass'] = status == 0 and report['result']['pass'] is True
                except (ValueError, KeyError, TypeError) as error:
                    report['error'] = 'Invalid result marker: ' + str(error)
            reports.append(report)
            print(json.dumps(report), flush=True)
            if not report['pass']:
                break
        if args.restart_url:
            report = {'pass': len(reports) == len(urls) and all(row['pass'] for row in reports),
                      'application': args.application, 'runtime': str(source_runtime),
                      'mode': 'probe-sequence', 'runs': reports}
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
