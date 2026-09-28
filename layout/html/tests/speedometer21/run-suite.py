#!/usr/bin/env python3
"""Run a content probe or unmodified Speedometer 2.1 in a disposable Suite profile."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from xml.sax.saxutils import quoteattr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--url', required=True)
    parser.add_argument('--mode', choices=['probe', 'benchmark'], default='benchmark')
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    environment = dict(os.environ, LD_LIBRARY_PATH=str(runtime),
                       MOZILLA_FIVE_HOME=str(runtime), MOZ_NO_REMOTE='1')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='zool-speedometer-') as temporary:
        base = Path(temporary)
        name = base.name

        def shell(source):
            script = base / 'profile.js'
            script.write_text(source)
            return subprocess.check_output([str(runtime / 'xpcshell'), '-f', str(script)],
                                           env=environment, text=True, timeout=30)

        info = None
        try:
            output = shell('''var p=Components.classes['@mozilla.org/profile/manager;1'].getService(Components.interfaces.nsIProfile);
var old=p.currentProfile;
if(p.profileExists(NAME))throw Error('Test profile already exists');
p.createNewProfile(NAME,BASE,null,false);
print('PROFILE='+JSON.stringify({original:old,path:p.QueryInterface(Components.interfaces.nsIProfileInternal).getProfileDir(NAME).path}));
'''.replace('NAME', json.dumps(name)).replace('BASE', json.dumps(str(base))))
            info = json.loads(next(line[8:] for line in output.splitlines() if line.startswith('PROFILE=')))
            profile = Path(info['path'])
            preferences = {
                'browser.dom.window.dump.enabled': True,
                'browser.shell.checkDefaultBrowser': False,
                'browser.startup.homepage_override.mstone': 'ignore',
                'nglayout.debug.disable_xul_cache': True,
                'nglayout.debug.disable_xul_fastload': True,
                'zoolrunner.speedometer.url': args.url,
                'zoolrunner.speedometer.mode': args.mode,
                'zoolrunner.speedometer.timeout': args.timeout,
            }
            (profile / 'user.js').write_text(''.join(
                'user_pref(' + json.dumps(k) + ',' + json.dumps(v) + ');\n'
                for k, v in preferences.items()))
            chrome = profile / 'chrome'
            chrome.mkdir(exist_ok=True)
            fixture = Path(__file__).resolve().parent.as_uri() + '/'
            (chrome / 'chrome.rdf').write_text('''<?xml version="1.0"?>
<RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:c="http://www.mozilla.org/rdf/chrome#">
<RDF:Seq RDF:about="urn:mozilla:package:root"><RDF:li RDF:resource="urn:mozilla:package:speedometer21"/></RDF:Seq>
<RDF:Description RDF:about="urn:mozilla:package:speedometer21" c:name="speedometer21" c:locType="profile" c:baseURL=BASE/>
</RDF:RDF>
'''.replace('BASE', quoteattr(fixture)))
            command = [str(runtime / 'zoolrunner-bin'), '-P', name,
                       '-chrome', 'chrome://speedometer21/content/runner.xul']
            with args.report.with_suffix('.log').open('w') as log:
                try:
                    result = subprocess.run(command, env=environment, stdout=log,
                                            stderr=subprocess.STDOUT, timeout=args.timeout + 60)
                    exit_code = result.returncode
                except subprocess.TimeoutExpired:
                    exit_code = 'timeout'
            output = args.report.with_suffix('.log').read_text(errors='replace')
            markers = [line[len('SPEEDOMETER-RESULT '):] for line in output.splitlines()
                       if line.startswith('SPEEDOMETER-RESULT ')]
            report = {'pass': False, 'exit': exit_code, 'runtime': str(runtime),
                      'url': args.url, 'mode': args.mode}
            if len(markers) == 1:
                report['result'] = json.loads(markers[0])
                report['pass'] = exit_code == 0 and report['result']['pass'] is True
            args.report.write_text(json.dumps(report, indent=2) + '\n')
            print(output)
            return 0 if report['pass'] else 1
        finally:
            if info:
                shell('''var C=Components.classes,I=Components.interfaces;
var r=C['@mozilla.org/registry;1'].createInstance(I.nsIRegistry);
r.open(C['@mozilla.org/file/directory_service;1'].getService(I.nsIProperties).get('AppRegF',I.nsIFile));
var k=r.getKey(I.nsIRegistry.Common,'Profiles');
if(r.getString(k,'CurrentProfile')==NAME){r.setString(k,'CurrentProfile',ORIGINAL);r.flush();}
var p=C['@mozilla.org/profile/manager;1'].getService(I.nsIProfile);
if(p.profileExists(NAME))p.deleteProfile(NAME,false);
'''.replace('NAME', json.dumps(name)).replace('ORIGINAL', json.dumps(info['original'])))


if __name__ == '__main__':
    raise SystemExit(main())
