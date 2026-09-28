#!/usr/bin/env python3
"""Run Suite application lifecycle checks in a temporary profile on a desktop."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from xml.sax.saxutils import quoteattr

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', type=Path, required=True, help='Suite dist/bin or Contents/MacOS directory')
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
runtime = args.runtime.absolute()
environment = dict(os.environ, DYLD_LIBRARY_PATH=str(runtime), MOZ_NO_REMOTE='1')
with tempfile.TemporaryDirectory(prefix='zool-lifecycle-') as temporary:
    base = Path(temporary)
    name = base.name
    def shell(source):
        script = base / 'profile-helper.js'
        script.write_text(source)
        return subprocess.check_output([str(runtime / 'xpcshell'), '-f', str(script)],
                                       env=environment, text=True, timeout=30)
    setup = '''var p=Components.classes['@mozilla.org/profile/manager;1'].getService(Components.interfaces.nsIProfile);
var old=p.currentProfile;
if(p.profileExists(NAME))throw Error('Test profile already exists');
p.createNewProfile(NAME,BASE,null,false);
print('PROFILE='+JSON.stringify({original:old,path:p.QueryInterface(Components.interfaces.nsIProfileInternal).getProfileDir(NAME).path}));
'''.replace('NAME', json.dumps(name)).replace('BASE', json.dumps(str(base)))
    info = None
    try:
        output = shell(setup)
        info = json.loads(next(line[8:] for line in output.splitlines() if line.startswith('PROFILE=')))
        profile = Path(info['path'])
        (profile / 'user.js').write_text('user_pref("browser.dom.window.dump.enabled",true);\n'
                                       'user_pref("nglayout.debug.disable_xul_cache",true);\n'
                                       'user_pref("nglayout.debug.disable_xul_fastload",true);\n')
        chrome = profile / 'chrome'
        chrome.mkdir(exist_ok=True)
        fixture = Path(__file__).resolve().parent
        (chrome / 'chrome.rdf').write_text('''<?xml version="1.0"?>
<RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:c="http://www.mozilla.org/rdf/chrome#">
<RDF:Seq RDF:about="urn:mozilla:package:root"><RDF:li RDF:resource="urn:mozilla:package:lifecycle"/></RDF:Seq>
<RDF:Description RDF:about="urn:mozilla:package:lifecycle" c:name="lifecycle" c:locType="profile" c:baseURL=BASE/>
</RDF:RDF>
'''.replace('BASE', quoteattr(fixture.as_uri() + '/')))
        args.report.parent.mkdir(parents=True, exist_ok=True)
        try:
            result = subprocess.run([str(runtime / 'zoolrunner-bin'), '-P', name,
                                     '-chrome', 'chrome://lifecycle/content/platform-lifecycle.xul'],
                                    env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, timeout=90)
        except subprocess.TimeoutExpired as error:
            output = error.stdout or ''
            if isinstance(output, bytes):
                output = output.decode('utf-8', errors='replace')
            args.report.write_text(output)
            print(output)
            raise
        args.report.write_text(result.stdout)
        print(result.stdout)
        if result.returncode or 'PLATFORM-LIFECYCLE checks=24 failures=0' not in result.stdout:
            raise SystemExit('Suite lifecycle regression failed')
    finally:
        if info:
            # Suite selects its profile on startup. Restore only if our test is
            # still selected, without overwriting a concurrent user's selection.
            cleanup = '''var C=Components.classes,I=Components.interfaces;
var r=C['@mozilla.org/registry;1'].createInstance(I.nsIRegistry);
r.open(C['@mozilla.org/file/directory_service;1'].getService(I.nsIProperties).get('AppRegF',I.nsIFile));
var k=r.getKey(I.nsIRegistry.Common,'Profiles');
if(r.getString(k,'CurrentProfile')==NAME){r.setString(k,'CurrentProfile',ORIGINAL);r.flush();}
var p=C['@mozilla.org/profile/manager;1'].getService(I.nsIProfile);
if(p.profileExists(NAME))p.deleteProfile(NAME,false);
print('Temporary profile removed');
'''.replace('NAME', json.dumps(name)).replace('ORIGINAL', json.dumps(info['original']))
            print(shell(cleanup))
