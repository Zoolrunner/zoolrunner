#!/usr/bin/env python3
"""Run the pinned historical ES5 Test262 suite in an existing xpcshell build."""
import argparse
import concurrent.futures
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', required=True, type=Path)
    parser.add_argument('--shell', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--filter', default='', help='substring of the suite path')
    parser.add_argument('--unmarked-default', choices=['non_strict', 'strict', 'both'],
                        default='non_strict',
                        help='upstream mode policy; both is an additional diagnostic run')
    parser.add_argument('--timezone', default='America/Los_Angeles',
                        help='timezone required by the historical fixed-date cases')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout <= 0:
        parser.error('jobs and timeout must be positive')
    suite, shell = args.suite.resolve(), args.shell.absolute()
    spec = importlib.util.spec_from_file_location(
        'test262_metadata', suite / 'tools/packaging/parseTestRecord.py')
    metadata = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(metadata)
    harness_dir = suite / 'test/harness'
    harness = '\n'.join((harness_dir / name).read_text(encoding='utf-8-sig')
                        for name in ['cth.js', 'sta.js', 'ed.js',
                                     'testBuiltInObject.js'])
    cases = []
    for path in sorted((suite / 'test/suite').glob('ch*/**/*.js')):
        name = path.relative_to(suite / 'test/suite').as_posix()
        if args.filter not in name:
            continue
        record = metadata.parseTestRecord(path.read_text(encoding='utf-8-sig'), name)
        for strict in [False, True]:
            if (strict and 'noStrict' in record) or (not strict and 'onlyStrict' in record):
                continue
            # This pinned branch defaults unmarked tests to non-strict. Its
            # older Sputnik cases are not all strict-compatible (upstream
            # tools/packaging/test262.py, BuildOptions/EnumerateTests).
            if 'noStrict' not in record and 'onlyStrict' not in record:
                mode = 'strict' if strict else 'non_strict'
                if args.unmarked_default not in ['both', mode]:
                    continue
            cases.append((name, strict, record))
    if not cases:
        parser.error('no matching ES5 tests')
    env = os.environ.copy()
    # Keep the dist/bin path, whose adjacent libraries are the installed ones.
    library_dir = str(shell.parent)
    for variable in ['DYLD_LIBRARY_PATH', 'LD_LIBRARY_PATH']:
        env[variable] = library_dir + (os.pathsep + env[variable] if env.get(variable) else '')
    env['MOZ_NO_REMOTE'] = '1'
    env['TZ'] = args.timezone

    def run(case):
        name, strict, record = case
        result = {'test': name, 'strict': strict}
        with tempfile.TemporaryDirectory(prefix='zoolrunner-test262-') as work:
            source = Path(work) / 'case.js'
            driver = Path(work) / 'driver.js'
            prefix = '"use strict";\nvar strict_mode = true;\n' if strict else 'var strict_mode = false;\n'
            setup = prefix + harness + '\n'
            # Keep a test's own directive prologue at the start of its script.
            code = ('"use strict";\n' if strict else '') + record['test'] + '\n'
            source.write_text(code, encoding='utf-8')
            # An ASCII transport string preserves every UTF-16 code unit;
            # evaluate compiles a global script, not an eval activation.
            driver.write_text('evaluate(' + json.dumps(setup, ensure_ascii=True) +
                              ', "test262-harness.js");\n' +
                              'try { evaluate(' + json.dumps(code, ensure_ascii=True) +
                              ', ' + json.dumps(str(source)) + ');\n'
                              'print("ZOOL262 PASS");\n'
                              '} catch (e) { print("ZOOL262 THROW " + String(e)); }\n', encoding='utf-8')
            try:
                proc = subprocess.run([str(shell), '-f', str(driver)], env=env,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      timeout=args.timeout)
            except subprocess.TimeoutExpired:
                return dict(result, status='timeout')
            output = proc.stdout.decode('utf-8', errors='replace')
            if proc.returncode:
                return dict(result, status='crash' if proc.returncode < 0 else 'harness-error',
                            detail=output[-2000:], exit_code=proc.returncode)
            match = re.search(r'^ZOOL262 (PASS|THROW)(.*)$', output, re.MULTILINE)
            if not match:
                return dict(result, status='harness-error', detail=output[-2000:])
            thrown = match.group(1) == 'THROW'
            detail = match.group(2).strip()
            if 'negative' in record:
                pattern = record['negative'] or '.'
                passed = thrown and re.search(pattern, detail) is not None
            else:
                passed = not thrown
            return dict(result, status='pass' if passed else 'fail', detail=detail)

    # A missing primitive needed by harness initialization must not turn every
    # test into a misleading language failure (or a passing negative test).
    for strict in [False, True]:
        preflight = run(('harness-preflight', strict, {'test':
            'if ("𐒠".length !== 2 || "𐒠".charCodeAt(0) !== 0xD801 || '
            '"𐒠".charCodeAt(1) !== 0xDCA0) throw Error("Unicode source transport");'}))
        if preflight['status'] != 'pass':
            parser.error('harness initialization failed: ' + str(preflight))
    started = time.monotonic()
    results = []
    print('Running %d test/mode cases' % len(cases), flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(run, cases):
            results.append(result)
            if len(results) % 500 == 0:
                print('%d/%d; passed %d' % (len(results), len(cases),
                                           sum(r['status'] == 'pass' for r in results)), flush=True)
    counts = {status: sum(r['status'] == status for r in results)
              for status in ['pass', 'fail', 'timeout', 'crash', 'harness-error']}
    revision = subprocess.check_output(['git', '-C', str(suite), 'rev-parse', 'HEAD'], text=True).strip()
    report = {'timezone': args.timezone, 'harness_layout': 'separate-global-script', 'unmarked_default': args.unmarked_default, 'source_transport': 'unicode-global-script', 'suite_revision': revision, 'shell': str(shell), 'filter': args.filter,
              'seconds': round(time.monotonic() - started, 2), 'counts': counts, 'results': results}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(counts), flush=True)
    return 0 if counts['pass'] == len(results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
