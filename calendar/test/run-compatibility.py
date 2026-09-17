#!/usr/bin/env python3
"""Run the unchanged Calendar unit tests with an isolated classic XPCOM profile."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--shell', type=Path, required=True)
parser.add_argument('--library-path', type=Path, help='Library directory for an uninstalled developer shell')
parser.add_argument('--report-dir', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
shell = args.shell.absolute()
args.report_dir.mkdir(parents=True, exist_ok=True)
failed = []
tests = sorted((root / 'calendar/test/unit').glob('test_*.js'))
if not tests:
    raise SystemExit('No Calendar unit tests found')
for test in tests:
    with tempfile.TemporaryDirectory(prefix='zool-calendar-unit-') as temp:
        setup = Path(temp) / 'setup.js'
        # The Calendar tests use two assertion helpers absent from the older
        # bundled harness. Supply their usual semantics without changing tests.
        setup.write_text('''var unitDir=Components.classes['@mozilla.org/file/local;1'].createInstance(Components.interfaces.nsILocalFile);
unitDir.initWithPath(PATH);
var unitDirs=Components.classes['@mozilla.org/file/directory_service;1'].getService(Components.interfaces.nsIProperties);
unitDirs.set('ProfD',unitDir);unitDirs.set('TmpD',unitDir);
function do_check_true(value){if(!value)do_throw('expected truthy value');}
function do_check_false(value){if(value)do_throw('expected falsy value');}
'''.replace('PATH', json.dumps(temp)))
        command = [str(shell), '-v', '170']
        for source in (root / 'tools/test-harness/xpcshell-simple/head.js', setup,
                       root / 'calendar/test/unit/head_consts.js', test,
                       root / 'tools/test-harness/xpcshell-simple/tail.js'):
            command += ['-f', str(source)]
        environment = dict(os.environ)
        if args.library_path:
            environment["DYLD_LIBRARY_PATH"] = str(args.library_path.absolute())
        try:
            result = subprocess.run(command, env=environment, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, universal_newlines=True, timeout=60)
            output = result.stdout
            ok = result.returncode == 0 and '*** PASS ***' in output and '*** FAIL ***' not in output
        except subprocess.TimeoutExpired as error:
            output = 'TIMEOUT\n' + (error.stdout or b'').decode(errors='replace')
            ok = False
        (args.report_dir / (test.stem + '.log')).write_text(output)
        print(('PASS ' if ok else 'FAIL ') + test.name, flush=True)
        if not ok:
            failed.append(test.name)
            print(output, flush=True)
print('CALENDAR-COMPATIBILITY tests=%d failures=%d' % (len(tests), len(failed)))
raise SystemExit(bool(failed))
