#!/usr/bin/env python3
"""Check a relocated engine and its internal probes using production objects."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

REVISION = '5e653f2e6ca14ac1ad8e801955a709cae7ac8a11'
# These unit probes deliberately use private engine interfaces. Link the exact
# objects from the application build into standalone probes, not a second JS
# engine inside an application and not new exports from the packaged library.
INTERNAL = {'TestClassRuntime', 'TestGlobalLexicalStore', 'TestMethodHome',
            'TestSuperReference', 'TestTypedArrays'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixtures(root):
    # Share the existing package fixture table. AST inspection does not execute
    # the macOS packager or introduce a second, drifting list of expectations.
    focused, native = {}, {}
    tree = ast.parse((root / 'build/macosx/package-ci.py').read_text())
    for node in ast.walk(tree):
        if not isinstance(node, ast.Tuple) or len(node.elts) != 2:
            continue
        try:
            name, marker = ast.literal_eval(node)
        except (ValueError, TypeError):
            continue
        if not isinstance(name, str) or not isinstance(marker, str):
            continue
        if name.endswith('.js') and (root / 'js/tests/es6' / name).is_file():
            focused[name] = marker
        if name.endswith('.c') and (root / 'js/tests' / name).is_file():
            native[name] = marker
    if not focused or not native or not INTERNAL <= {Path(n).stem for n in native}:
        raise RuntimeError('Incomplete engine fixture table')
    return focused, native


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('arch', choices=['i686', 'x86_64', 'aarch64'])
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--objdir', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--logs', type=Path, required=True)
    parser.add_argument('--suite', type=Path)
    args = parser.parse_args()
    root, obj, runtime, logs = [p.resolve() for p in
                              (args.root, args.objdir, args.runtime, args.logs)]
    logs.mkdir(parents=True, exist_ok=True)
    engine = runtime / 'libmozjs.so'
    before = digest(engine)
    if before != digest(obj / 'dist/bin/libmozjs.so'):
        raise RuntimeError('Package engine differs from the build providing unit-test objects')
    env = dict(os.environ, LD_LIBRARY_PATH=str(runtime),
               MOZILLA_FIVE_HOME=str(runtime), MOZ_NO_REMOTE='1',
               TZ='America/Los_Angeles')
    focused, native = fixtures(root)
    results = []

    def run(command, label, marker=None, timeout=180, data=None, cwd=None):
        command = [str(x) for x in command]
        timed_out = False
        with (logs / (label + '.log')).open('w') as output:
            try:
                result = subprocess.run(command, env=env, cwd=str(cwd or logs),
                                        input=data, universal_newlines=True,
                                        stdout=output, stderr=subprocess.STDOUT,
                                        timeout=timeout)
                code = result.returncode
            except subprocess.TimeoutExpired:
                timed_out, code = True, None
        text = (logs / (label + '.log')).read_text(errors='replace')
        passed = not timed_out and code == 0 and (marker is None or marker in text)
        results.append(dict(label=label, command=command, passed=passed,
                            exit=code, timeout=timed_out))
        (logs / 'checks.json').write_text(json.dumps(results, indent=2) + '\n')
        return passed

    shell = runtime / 'xpcshell'
    if not run([shell, '-e', 'print("ES6-RUNTIME READY")'], 'startup', 'ES6-RUNTIME READY'):
        raise RuntimeError('Packaged engine did not initialize')
    for name, marker in sorted(focused.items()):
        modern = not name.startswith('../')
        command = [shell] + (['-E', '-v', '2015'] if modern else [])
        run(command + ['-f', root / 'js/tests/es6' / name],
            Path(name).name + ('-modern' if modern else '-legacy'), marker)

    with tempfile.TemporaryDirectory(prefix='zool-es6-native-') as temporary:
        base = Path(temporary)
        core = obj / 'js/src'
        recipe = '.PHONY: zr-es6-objects\nzr-es6-objects:\n\t@printf "%s\\n" "ES6-OBJECTS: $(OBJS)"\n'
        if not run(['make', '--no-print-directory', '-s', '-f', 'Makefile', '-f', '-',
                    'zr-es6-objects'], 'engine-objects', 'ES6-OBJECTS:', data=recipe, cwd=core):
            raise RuntimeError('Cannot obtain production engine object list')
        line, = [line for line in (logs / 'engine-objects.log').read_text().splitlines()
                 if line.startswith('ES6-OBJECTS:')]
        objects = [(core / item).resolve() for item in shlex.split(line.split(':', 1)[1])]
        if not objects or any(not path.is_file() or path.suffix != '.o' for path in objects):
            raise RuntimeError('Missing production engine objects')
        object_hashes = {str(path.relative_to(obj)): digest(path) for path in objects}
        (logs / 'engine-objects.json').write_text(json.dumps(object_hashes, indent=2) + '\n')
        archive = base / 'engine-internal.a'
        if not run(['ar', 'crs', archive] + objects, 'engine-archive'):
            raise RuntimeError('Cannot archive production engine objects')
        for name, marker in sorted(native.items()):
            stem = Path(name).stem
            binary = base / stem
            internal = stem in INTERNAL
            command = ['gcc'] + (['-m32', '-march=i686'] if args.arch == 'i686' else [])
            command += ['-std=gnu89', '-DXP_UNIX', '-DJS_THREADSAFE',
                        '-DMOZILLA_1_8_BRANCH', '-I' + str(root / 'js/src'),
                        '-I' + str(core), '-I' + str(obj / 'dist/include/nspr'),
                        root / 'js/tests' / name, '-L' + str(runtime),
                        '-Wl,-rpath-link,' + str(runtime)]
            command += [archive] if internal else ['-lmozjs']
            command += ['-lplds4', '-lplc4', '-lnspr4', '-lm', '-pthread', '-ldl', '-o', binary]
            if run(command, stem + '-build'):
                run([binary], stem + ('-internal' if internal else '-packaged'), marker)

        suite = args.suite.resolve() if args.suite else base / 'test262'
        if args.suite is None:
            for command, label in [(['git', 'init', suite], 'suite-init'),
                                   (['git', '-C', suite, 'fetch', '--depth=1',
                                     'https://github.com/tc39/test262.git', REVISION], 'suite-fetch'),
                                   (['git', '-C', suite, 'checkout', '--detach', REVISION], 'suite-checkout')]:
                if not run(command, label, timeout=600):
                    raise RuntimeError('Cannot prepare pinned ES2015 corpus')
        report = logs / 'test262.json'
        full_pass = run(['python3', root / 'js/tests/es6/run-test262.py', '--suite', suite,
                         '--shell', shell, '--report', report, '--jobs', '4',
                         '--timeout', '60'], 'test262', timeout=1800)
        counts = json.loads(report.read_text())['counts'] if report.exists() else None
        expected = {'pass': 28582, 'fail': 0, 'unsupported': 0,
                    'timeout': 0, 'crash': 0, 'harness-error': 0}
        unchanged = before == digest(engine)
        passed = full_pass and counts == expected and unchanged and all(x['passed'] for x in results)
        summary = dict(passed=passed, arch=args.arch, timezone=env['TZ'],
                       focused=len(focused), native=len(native),
                       internal_native=len(INTERNAL), counts=counts,
                       engine_sha256=before, engine_unchanged=unchanged)
        (logs / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
        if not passed:
            raise RuntimeError('Incomplete ES2015 validation; see ' + str(logs))
        print('ES2015 PASS: ' + args.arch + ' all pinned modes and focused/native probes')


if __name__ == '__main__':
    main()
