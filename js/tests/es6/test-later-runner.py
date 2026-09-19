#!/usr/bin/env python3
"""Phase/isolation controls for the later diagnostic host; needs --shell."""
import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile

spec = importlib.util.spec_from_file_location('later', Path(__file__).with_name('diagnose-later-test262.py'))
later = importlib.util.module_from_spec(spec)
spec.loader.exec_module(later)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--shell', type=Path, required=True)
    args = parser.parse_args()
    shell = args.shell.resolve()
    checks = 0
    def control(source, expected, negative=None, harness='', mode='non-strict', flags=()):
        nonlocal checks
        record = dict(flags=list(flags))
        if negative:
            record['negative'] = dict(phase=negative[0], type=negative[1])
        case = dict(source=source, mode=mode, test='control.js', record=record)
        with tempfile.TemporaryDirectory(prefix='zool-later-control-') as temporary:
            path = Path(temporary) / 'control.js'
            path.write_text(later.phase_driver(case, harness, 'CONTROL '))
            proc = subprocess.run([str(shell), '-E', '-v', '2015', '-f', str(path)],
                                  env=dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent)),
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=15)
            status, detail = later.classify(case, proc.stdout, proc.returncode, 'CONTROL ')
            assert status == expected, (source, status, detail)
        checks += 1
    control('var =', 'pass', ('parse', 'SyntaxError'))
    control('throw new SyntaxError("runtime")', 'fail', ('parse', 'SyntaxError'))
    control('throw new SyntaxError("runtime")', 'pass', ('runtime', 'SyntaxError'))
    control('var =', 'fail', ('runtime', 'SyntaxError'))
    control('throw new TypeError("wrong type")', 'fail', ('runtime', 'SyntaxError'))
    control('throw new SyntaxError("body")', 'harness-error', ('runtime', 'SyntaxError'), harness='var =')
    control('"use strict"; undeclaredControl=1', 'pass', ('runtime', 'ReferenceError'), harness='var harnessValue=1')
    control('var \u03c0="\U0001f600";if(\u03c0.length!==2)throw Error("transport")', 'pass')
    control('Object.preventExtensions(this)', 'pass')
    control('$262.createRealm().evalScript("var isolated=1");if(typeof isolated!=="undefined")throw Error("realm")', 'pass')
    control('export var =', 'pass', ('parse', 'SyntaxError'), mode='module')
    control('export var x=1;throw new TypeError("module runtime")', 'pass', ('runtime', 'TypeError'), mode='module')
    control('import "unprovided.js"', 'harness-error', ('resolution', 'SyntaxError'), mode='module')
    control('Promise.resolve().then(function(){$DONE()})', 'pass', flags=['async'])
    control('$DONE();$DONE()', 'fail', flags=['async'])
    control('$DONE(new TypeError("async"))', 'pass', ('runtime', 'TypeError'), flags=['async'])
    print('LATER-RUNNER-CONTROLS checks=' + str(checks) + ' failures=0')


if __name__ == '__main__':
    main()
