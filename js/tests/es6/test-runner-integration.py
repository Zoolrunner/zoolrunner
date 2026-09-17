#!/usr/bin/env python3
"""Exercise source transport and result accounting with a real xpcshell."""
import argparse
import importlib.util
import os
from pathlib import Path
import tempfile

spec = importlib.util.spec_from_file_location('runner', Path(__file__).with_name('run-test262.py'))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--shell', type=Path, required=True)
p.add_argument('--suite', type=Path, required=True)
args = p.parse_args()
shell = args.shell.absolute()
env = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent), LD_LIBRARY_PATH=str(shell.parent))
fixtures = [
    ('unicode', 'strict', {}, 'if ("𐒠".charCodeAt(1) !== 0xDCA0) throw Error("transport");', 'pass'),
    ('own-directive', 'non-strict', {}, '"use strict"; if ((function(){return this;})() !== undefined) throw Error("directive");', 'pass'),
    ('global-bookkeeping', 'strict', {}, 'var emit = null, compile = null, stringify = null, done = 42;', 'pass'),
    ('parse-negative', 'strict', {'negative': 'SyntaxError'}, 'var public;', 'pass'),
    ('wrong-negative', 'strict', {'negative': 'SyntaxError'}, 'throw new TypeError("wrong");', 'fail'),
    ('harness-negative', 'strict', {'negative': '.*', 'includes': ['missing.js']}, '', 'harness-error'),
    ('async-complete', 'strict', {'flags': ['async']}, '$DONE();', 'pass'),
    ('async-error', 'strict', {'flags': ['async']}, '$DONE(new Error("bad"));', 'fail'),
    ('async-duplicate', 'strict', {'flags': ['async']}, '$DONE(); $DONE();', 'fail'),
    ('async-pending', 'strict', {'flags': ['async']}, '', 'unsupported'),
    ('module', 'module', {}, 'export default 1;', 'unsupported'),
    ('early-exit', 'raw', {}, 'quit(0);', 'harness-error'),
    ('timeout', 'raw', {}, 'while (true) {}', 'timeout'),
]
for name, mode, record, source, expected in fixtures:
    case = dict(test=name, mode=mode, record=record, source=source)
    result = runner.run_case(case, args.suite, shell, env, 2)
    if result['status'] != expected:
        raise RuntimeError('%s expected %s: %r' % (name, expected, result))
with tempfile.TemporaryDirectory(prefix='zoolrunner-es6-harness-') as temporary:
    suite = Path(temporary)
    harness = suite / 'harness'
    harness.mkdir()
    for name in ('sta.js', 'cth.js', 'assert.js'):
        (harness / name).write_text('throw new SyntaxError("harness error");')
    case = dict(test='harness-exception', mode='strict',
                record={'negative': 'SyntaxError'}, source='var public;')
    result = runner.run_case(case, suite, shell, env, 2)
    if result['status'] != 'harness-error':
        raise RuntimeError('Harness exception satisfied negative test: %r' % result)
print('ES6-RUNNER checks=%d failures=0' % (len(fixtures) + 1))
