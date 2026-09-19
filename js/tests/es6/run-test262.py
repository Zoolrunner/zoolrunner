#!/usr/bin/env python3
"""Run the pinned ES2015-era Test262 corpus without hiding unsupported cases."""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

import yaml

REVISION = '5e653f2e6ca14ac1ad8e801955a709cae7ac8a11'
ROOTS = ('annexB', 'built-ins', 'language')
STATUSES = ('pass', 'fail', 'unsupported', 'timeout', 'crash', 'harness-error')
FULL_FILES = 14968
FULL_CASES = 28582


def runtime_hashes(shell):
    paths = [shell]
    for pattern in ('*mozjs*.dylib', '*mozjs*.so*', '*js*.dll'):
        paths.extend(sorted(shell.parent.glob(pattern)))
    return {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in paths if path.is_file()}


def records(suite, selection=''):
    for directory in ROOTS:
        for path in sorted((suite / 'test' / directory).rglob('*.js')):
            name = path.relative_to(suite / 'test').as_posix()
            if selection not in name:
                continue
            source = path.read_text(encoding='utf-8-sig')
            match = re.search(r'/\*---(.*?)---\*/', source, re.S)
            if not match:
                raise ValueError('Missing Test262 metadata: ' + name)
            record = yaml.safe_load(match.group(1))
            flags = set(record.get('flags', []))
            unknown = flags - {'noStrict', 'onlyStrict', 'raw', 'module', 'async'}
            if unknown or {'noStrict', 'onlyStrict'} <= flags:
                raise ValueError('Invalid/unknown flags in ' + name + ': ' + repr(flags))
            modes = ['module'] if 'module' in flags else (
                ['raw'] if 'raw' in flags else ['non-strict', 'strict'])
            for mode in modes:
                if mode == 'strict' and 'noStrict' in flags:
                    continue
                if mode == 'non-strict' and 'onlyStrict' in flags:
                    continue
                yield dict(test=name, mode=mode, record=record, source=source)


def driver_source(case, harness, marker):
    source = ('"use strict";\n' if case['mode'] == 'strict' else '') + case['source']
    asynchronous = 'async' in case['record'].get('flags', []) or '$DONE' in case['source']
    # A function scope keeps runner bookkeeping out of the tested global.
    # evaluate compiles Unicode source as a separate *global* script, not eval.
    return '''(function (global) {
var emit = print, compile = evaluate, stringify = String, done = 0, doneError;
var compileModuleUnit = typeof compileModule === "function" ? compileModule : null;
var evaluateModuleUnit = typeof evaluateModule === "function" ? evaluateModule : null;
var drain = typeof drainJobQueue === "function" ? drainJobQueue : null;
function describe(error) {
  try { return stringify(error); } catch (_) { return "unprintable exception"; }
}
function finish(kind, detail) { emit(%s + kind + " " + (detail || "")); }
try { compile(%s, "test262-harness.js"); }
catch (error) { finish("HARNESS", describe(error)); return; }
if (%s) global.$DONE = function (error) { done++; if (error !== undefined) doneError = error; };
try {
  if (%s) {
    if (!compileModuleUnit || !evaluateModuleUnit) {
      finish("UNSUPPORTED", "module host APIs are unavailable"); return;
    }
    evaluateModuleUnit(compileModuleUnit(%s, %s));
  } else { compile(%s, %s); }
}
catch (error) { finish("THROW", describe(error)); return; }
// A queued exception cannot satisfy a synchronous negative-test pattern.
try { if (drain) drain(); }
catch (error) { finish("FAIL", "job checkpoint: " + describe(error)); return; }
if (%s) {
  if (done > 1) { finish("FAIL", "$DONE called more than once"); return; }
  if (doneError !== undefined) { finish("FAIL", describe(doneError)); return; }
  if (!done) { finish("UNSUPPORTED", "async test did not complete at the host job checkpoint"); return; }
}
finish("PASS", "");
})(this);
''' % (json.dumps(marker), json.dumps(harness, ensure_ascii=True),
       'true' if asynchronous else 'false', 'true' if case['mode'] == 'module' else 'false',
       json.dumps(source, ensure_ascii=True), json.dumps(case['test']),
       json.dumps(source, ensure_ascii=True), json.dumps(case['test']),
       'true' if asynchronous else 'false')


def classify(case, output, code, marker):
    if code:
        return ('crash' if code < 0 else 'harness-error', output[-3000:])
    matches = re.findall(r'^' + re.escape(marker) + r'(\w+) ?(.*)$', output, re.M)
    if len(matches) != 1:
        return 'harness-error', 'Missing/ambiguous completion marker\n' + output[-3000:]
    kind, detail = matches[0]
    if kind == 'HARNESS':
        return 'harness-error', detail
    if kind == 'UNSUPPORTED':
        return 'unsupported', detail
    negative = case['record'].get('negative')
    if negative is not None:
        if not isinstance(negative, str):
            return 'harness-error', 'Unsupported negative metadata: ' + repr(negative)
        passed = kind == 'THROW' and re.search(negative or '.', detail) is not None
    else:
        passed = kind == 'PASS'
    return ('pass' if passed else 'fail'), detail


def run_case(case, suite, shell, env, timeout, edition='es2015'):
    result = {key: case[key] for key in ('test', 'mode')}
    result['features'] = case['record'].get('features', [])
    result['specification'] = {key: case['record'][key]
                               for key in ('es5id', 'es6id', 'es7id', 'esid')
                               if key in case['record']}
    try:
        includes = [] if case['mode'] == 'raw' else ['sta.js', 'cth.js', 'assert.js']
        if case['mode'] != 'raw':
            includes += case['record'].get('includes', []) or re.findall(
                r'\$INCLUDE\([\'"]([^\'\"]+)[\'\"]\)', case['source'])
        harness = ''
        for name in includes:
            path = (suite / 'harness' / name).resolve()
            if path.parent != (suite / 'harness').resolve():
                raise ValueError('Invalid harness include: ' + name)
            harness += path.read_text(encoding='utf-8-sig') + '\n'
        with tempfile.TemporaryDirectory(prefix='zoolrunner-es6-') as tmp:
            marker = 'ZOOL262-' + Path(tmp).name + ' '
            driver = Path(tmp) / 'driver.js'
            driver.write_text(driver_source(case, harness, marker), encoding='utf-8')
            options = ['-E', '-v', '2015'] if edition == 'es2015' else ['-v', '0']
            proc = subprocess.run([str(shell)] + options + ['-f', str(driver)], env=env,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
            output = proc.stdout.decode('utf-8', errors='replace')
            status, detail = classify(case, output, proc.returncode, marker)
            return dict(result, status=status, detail=detail, exit_code=proc.returncode)
    except subprocess.TimeoutExpired:
        return dict(result, status='timeout', detail='Process exceeded time limit')
    except (OSError, ValueError) as error:
        return dict(result, status='harness-error', detail=str(error))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--suite', type=Path, required=True)
    p.add_argument('--shell', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--filter', default='', help='Diagnostic subset; never a full-suite pass')
    p.add_argument('--jobs', type=int, default=4)
    p.add_argument('--timeout', type=float, default=10)
    p.add_argument('--timezone', default='America/Los_Angeles')
    p.add_argument('--edition', choices=('es2015', 'legacy'), default='es2015',
                   help='ES2015 opt-in, or the historical default for baseline comparisons')
    args = p.parse_args()
    if args.jobs < 1 or args.timeout <= 0:
        p.error('jobs and timeout must be positive')
    suite, shell = args.suite.resolve(), args.shell.absolute()
    revision = subprocess.check_output(['git', '-C', str(suite), 'rev-parse', 'HEAD'],
                                       universal_newlines=True).strip()
    dirty = subprocess.check_output(['git', '-C', str(suite), 'status', '--porcelain'],
                                    universal_newlines=True)
    if revision != REVISION or dirty:
        p.error('The complete upstream checkout must be clean and pinned to ' + REVISION)
    env = dict(os.environ, TZ=args.timezone, MOZ_NO_REMOTE='1')
    env['DYLD_LIBRARY_PATH'] = str(shell.parent)
    env['LD_LIBRARY_PATH'] = str(shell.parent)
    preflight = dict(test='transport-preflight', mode='strict', record={}, source=
        'if ("𐒠".length !== 2 || "𐒠".charCodeAt(0) !== 0xD801 || '
        '"𐒠".charCodeAt(1) !== 0xDCA0) throw Error("Unicode transport");\n'
        'if ((function () { return this; })() !== undefined) throw Error("strict mode");')
    if args.edition == 'es2015':
        preflight['source'] += '\nif (version() !== 2015 || ({x:1,x:2}).x !== 2) '
        preflight['source'] += 'throw Error("ES2015 edition selection");'
        preflight['source'] += ('\nif (!Object.getOwnPropertyDescriptor(Number, '
                                '"length").configurable) '
                                'throw Error("ES2015 global initialization");')
    check = run_case(preflight, suite, shell, env, args.timeout, args.edition)
    if check['status'] != 'pass':
        p.error('Harness/Unicode/strict-mode preflight failed: ' + repr(check))
    cases = list(records(suite, args.filter))
    if not cases:
        p.error('No test cases selected')
    if not args.filter and (len(cases) != FULL_CASES or
                           len({c['test'] for c in cases}) != FULL_FILES):
        p.error('Incomplete pinned corpus; expected %d files / %d cases' %
                (FULL_FILES, FULL_CASES))
    binaries = runtime_hashes(shell)
    print('Running %d test/mode cases; unsupported cases remain failures to complete' % len(cases), flush=True)
    started = time.monotonic()
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        def run(case):
            return run_case(case, suite, shell, env, args.timeout, args.edition)
        for result in pool.map(run, cases):
            results.append(result)
            if len(results) % 500 == 0:
                print('%d/%d cases complete' % (len(results), len(cases)), flush=True)
    counts = {status: sum(r['status'] == status for r in results) for status in STATUSES}
    report = dict(suite_revision=revision, corpus='ES2015-era baseline',
                  roots=ROOTS, filter=args.filter, timezone=args.timezone,
                  full_selection=not args.filter, unmarked_default='both',
                  language_edition=args.edition, global_initialization=args.edition,
                  source_transport='unicode-global-script', harness_layout='separate-global-script',
                  shell=str(shell), shell_sha256=hashlib.sha256(shell.read_bytes()).hexdigest(),
                  runtime_sha256=binaries,
                  runtime_unchanged=runtime_hashes(shell) == binaries,
                  seconds=round(time.monotonic() - started, 2), counts=counts, results=results)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(counts), flush=True)
    return 0 if counts['pass'] == len(results) and report['runtime_unchanged'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
