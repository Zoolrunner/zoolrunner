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
    def control(source, expected, negative=None, harness='', mode='non-strict', flags=(), fixtures=None):
        nonlocal checks
        record = dict(flags=list(flags))
        if negative:
            record['negative'] = dict(phase=negative[0], type=negative[1])
        case = dict(source=source, mode=mode, test='control.js', record=record)
        with tempfile.TemporaryDirectory(prefix='zool-later-control-') as temporary:
            path = Path(temporary) / 'control.js'
            path.write_text(later.phase_driver(case, harness, 'CONTROL ', fixtures))
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
    control('import {x} from "./dep_FIXTURE.js";if(x!==7)throw Error("binding")', 'pass',
            mode='module', fixtures={'dep_FIXTURE.js': 'export let x=7;'})
    control('import {x} from "./dep_FIXTURE.js";import {x as y} from "./sub/../dep_FIXTURE.js";'
            'if(x!==y||counter!==1)throw Error("duplicate evaluation")', 'pass',
            harness='var counter=0;', mode='module',
            fixtures={'dep_FIXTURE.js': 'counter++;export const x={};'})
    control('import {f} from "./dep_FIXTURE.js";export function g(){return 9;}'
            'if(f()!==9)throw Error("cycle")', 'pass', mode='module',
            fixtures={'dep_FIXTURE.js': 'import {g} from "./control.js";export function f(){return g();}'})
    control('import "./dep_FIXTURE.js"', 'pass', ('resolution', 'SyntaxError'), mode='module',
            fixtures={'dep_FIXTURE.js': 'export var =;'})
    control('import "./dep_FIXTURE.js";export var =;', 'pass', ('parse', 'SyntaxError'), mode='module',
            fixtures={'dep_FIXTURE.js': 'export var =;'})
    control('import "./dep_FIXTURE.js"', 'pass', ('runtime', 'TypeError'), mode='module',
            fixtures={'dep_FIXTURE.js': 'throw new TypeError("dependency runtime");'})
    control('import "./dep_FIXTURE.js"', 'fail', ('resolution', 'TypeError'), mode='module',
            fixtures={'dep_FIXTURE.js': 'throw new TypeError("dependency runtime");'})
    control('import "./missing_FIXTURE.js"', 'harness-error', ('resolution', 'SyntaxError'), mode='module')
    control('Promise.resolve().then(function(){$DONE()})', 'pass', flags=['async'])
    control('$DONE();$DONE()', 'fail', flags=['async'])
    control('$DONE(new TypeError("async"))', 'pass', ('runtime', 'TypeError'), flags=['async'])
    # Exercise metadata validation too: generated describes file provenance,
    # whereas an unknown flag must remain an explicit harness failure.
    with tempfile.TemporaryDirectory(prefix='zool-later-flags-') as temporary:
        suite = Path(temporary)
        case = dict(source='var generatedControl=1;', mode='raw',
                    test='generated-control.js', sha256='control',
                    record=dict(flags=['raw', 'generated']))
        assert later.run_case(case, suite, shell, 15)['status'] == 'diagnostic-pass'
        checks += 1
        case['record']['flags'].append('unknownControlFlag')
        assert later.run_case(case, suite, shell, 15)['status'] == 'diagnostic-harness-error'
        checks += 1
    print('LATER-RUNNER-CONTROLS checks=' + str(checks) + ' failures=0')


if __name__ == '__main__':
    main()
