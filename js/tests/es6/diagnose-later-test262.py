#!/usr/bin/env python3
"""Diagnose es6id-tagged cases in the pinned later Test262 coverage inventory.

This is an explicitly bounded diagnostic, not a full ES2015 conformance run.
An es6id can survive later syntax and normative changes. Keep every selected
result visible; edition decisions belong in the separate coverage review.
"""
import argparse
import collections
import concurrent.futures
import functools
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

import yaml

REVISION = '35d566604512cba908054eec49f85e64a59f3091'
ROOTS = ('annexB', 'built-ins', 'language')
# Positive diagnostic selection only: mixed newer features are retained and
# all selected failures remain visible. These tags do not decide edition scope.
ES2015_FEATURES = set("""
Array.prototype.values ArrayBuffer DataView Float32Array Float64Array Int16Array
Int32Array Int8Array Map Object.is Promise Proxy Reflect Reflect.construct
Reflect.set Reflect.setPrototypeOf Set String.fromCodePoint String.prototype.endsWith
String.prototype.includes Symbol Symbol.hasInstance Symbol.isConcatSpreadable
Symbol.iterator Symbol.match Symbol.replace Symbol.search Symbol.species Symbol.split
Symbol.toPrimitive Symbol.toStringTag Symbol.unscopables TypedArray Uint16Array
Uint32Array Uint8Array Uint8ClampedArray WeakMap WeakSet arrow-function class
computed-property-names const default-parameters destructuring-assignment
destructuring-binding for-of generators let new.target rest-parameters super
tail-call-optimization template
""".split())

spec = importlib.util.spec_from_file_location('historical_runner', Path(__file__).with_name('run-test262.py'))
historical = importlib.util.module_from_spec(spec)
spec.loader.exec_module(historical)


@functools.lru_cache(maxsize=2)
def module_fixtures(suite):
    root = suite / 'test'
    return {path.relative_to(root).as_posix(): path.read_text(encoding='utf-8-sig')
            for path in sorted(root.rglob('*_FIXTURE.js'))}


def phase_driver(case, harness, marker, fixtures=None):
    source = ('"use strict";\n' if case['mode']=='strict' else '') + case['source']
    negative=case['record'].get('negative') or {}
    values=dict(source=source,harness=harness,marker=marker,filename=case['test'],
                module=case['mode']=='module',async_='async' in case['record'].get('flags',[]),
                expected=negative.get('type'),fixtures=fixtures or {})
    return '(function(p){\n'+r'''
var realm=createTest262Realm(), emit=print, encode=JSON.stringify, stringify=String;
var expected=p.expected ? realm.global[p.expected] : null;
var compile=realm.compileScript, execute=realm.executeScript;
var cm=realm.compileModule, im=realm.instantiateModule, em=realm.evaluateModule;
var done=0, doneError, unit;
function finish(kind, phase, error) {
  var detail='';try{detail=error===undefined?'':stringify(error)}catch(_){}
  var matches=false;try{matches=!!expected && error.constructor===expected}catch(_){}
  emit(p.marker+encode({kind:kind,phase:phase,matches:matches,detail:detail}));
}
if(p.async_) realm.global.$DONE=function(error){done++;if(error!==undefined)doneError=error;};
try{realm.evalScript(p.harness)}catch(e){finish('harness-error','harness',e);return;}
try{unit=p.module?cm(p.source,p.filename):compile(p.source)}
catch(e){finish('throw','parse',e);return;}
if(p.module){
  var units=Object.create(null), missing={}, missingName='';
  units[p.filename]=unit;
  function resolve(parent,request){
    if(request.slice(0,2)!=='./' && request.slice(0,3)!=='../'){
      missingName=request;throw missing;
    }
    var parts=parent.split('/');parts.pop();
    var rest=request.split('/');
    for(var i=0;i<rest.length;i++){
      if(rest[i]==='.' || rest[i]==='')continue;
      if(rest[i]==='..'){
        if(!parts.length){missingName=request;throw missing;}
        parts.pop();
      }else parts.push(rest[i]);
    }
    return parts.join('/');
  }
  function linkDependencies(current,name){
    var requests=moduleRequests(current);
    for(var i=0;i<requests.length;i++){
      var dependency=resolve(name,requests[i]);
      if(!Object.prototype.hasOwnProperty.call(units,dependency)){
        if(!Object.prototype.hasOwnProperty.call(p.fixtures,dependency)){
          missingName=dependency;throw missing;
        }
        var child=cm(p.fixtures[dependency],dependency);
        units[dependency]=child;
        linkDependencies(child,dependency);
      }
      realm.linkModule(current,requests[i],units[dependency]);
    }
  }
  try{linkDependencies(unit,p.filename)}catch(e){
    if(e===missing)finish('harness-error','resolution','Unprovided module dependency: '+missingName);
    else finish('throw','resolution',e);
    return;
  }
  try{im(unit)}catch(e){finish('throw','resolution',e);return;}
}
try{if(p.module)em(unit);else execute(unit)}
catch(e){finish('throw','runtime',e);return;}
try{drainJobQueue()}catch(e){finish('fail','checkpoint',e);return;}
if(p.async_){
  if(done!==1){finish('fail','checkpoint','Expected one $DONE call, got '+done);return;}
  if(doneError!==undefined){finish('throw','runtime',doneError);return;}
}
finish('pass','runtime');
'''+'})('+json.dumps(values,ensure_ascii=True)+');\n'


def records(suite, selection, category="es6id"):
    for directory in ROOTS:
        for path in sorted((suite / 'test' / directory).rglob('*.js')):
            name = path.relative_to(suite / 'test').as_posix()
            # Apply the existing path restriction before reading/parsing
            # unrelated metadata. This does not change the selected cases.
            if selection not in name:
                continue
            source = path.read_text(encoding='utf-8-sig')
            match = re.search(r'/\*---(.*?)---\*/', source, re.S)
            metadata = (yaml.safe_load(match.group(1)) or {}) if match else {}
            selected = ('es6id' in metadata if category == 'es6id' else
                        bool(ES2015_FEATURES.intersection(metadata.get('features', []))))
            if not selected:
                continue
            flags = set(metadata.get('flags', []))
            modes = ['module'] if 'module' in flags else ['raw'] if 'raw' in flags else (
                ['strict'] if 'onlyStrict' in flags else ['non-strict'] if 'noStrict' in flags else ['non-strict', 'strict'])
            for mode in modes:
                yield dict(test=name, source=source, record=metadata, mode=mode,
                           sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def classify(case, output, code, marker):
    if code:
        return ('crash' if code < 0 else 'harness-error'), output[-3000:]
    lines = [line[len(marker):] for line in output.splitlines() if line.startswith(marker)]
    if len(lines) != 1:
        return 'harness-error', 'Missing/ambiguous completion marker\n' + output[-3000:]
    outcome = json.loads(lines[0])
    if outcome['kind'] == 'harness-error':
        return 'harness-error', outcome
    negative = case['record'].get('negative')
    if negative:
        passed = (outcome['kind'] == 'throw' and outcome['phase'] == negative['phase'] and outcome['matches'])
    else:
        passed = outcome['kind'] == 'pass'
    return ('pass' if passed else 'fail'), outcome


def run_case(case, suite, shell, timeout):
    result = {key: case[key] for key in ('test', 'mode', 'sha256')}
    result.update(specification=case['record'].get('es6id'), features=case['record'].get('features', []), edition_review='pending',
                  negative_phase_verified=False)
    try:
        flags = set(case['record'].get('flags', []))
        # Upstream INTERPRETING.md defines generated as provenance only;
        # it imposes no execution-mode or host requirement.
        if flags - {'noStrict', 'onlyStrict', 'raw', 'module', 'async', 'generated'} or {'noStrict', 'onlyStrict'} <= flags:
            raise ValueError('Unknown/conflicting host flags: ' + repr(flags))
        negative = case['record'].get('negative')
        if negative and (not isinstance(negative, dict) or
                         negative.get('phase') not in ('parse', 'resolution', 'runtime') or
                         negative.get('type') not in ('SyntaxError', 'ReferenceError', 'TypeError', 'RangeError', 'EvalError', 'URIError', 'Error')):
            raise ValueError('Unrecognized negative metadata: ' + repr(negative))
        harness = ''
        if case['mode'] != 'raw':
            for name in ['assert.js', 'sta.js'] + case['record'].get('includes', []):
                path = (suite / 'harness' / name).resolve()
                if path.parent != (suite / 'harness').resolve():
                    raise ValueError('Invalid harness include: ' + name)
                harness += path.read_text(encoding='utf-8-sig') + '\n'
        with tempfile.TemporaryDirectory(prefix='zoolrunner-later-') as temporary:
            marker = 'LATER-' + Path(temporary).name + ' '
            driver = Path(temporary) / 'driver.js'
            fixtures = module_fixtures(suite) if case['mode'] == 'module' else None
            driver.write_text(phase_driver(case, harness, marker, fixtures))
            proc = subprocess.run([str(shell), '-E', '-v', '2015', '-f', str(driver)],
                                  env=dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent), TZ='America/Los_Angeles'),
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
            status, detail = classify(case, proc.stdout.decode('utf-8', errors='replace'), proc.returncode, marker)
            result['negative_phase_verified'] = bool(negative) and isinstance(detail, dict) and detail.get('phase') in ('parse', 'resolution', 'runtime')
    except subprocess.TimeoutExpired:
        status, detail = 'timeout', 'Process exceeded time limit'
    except (OSError, ValueError, KeyError) as error:
        status, detail = 'harness-error', str(error)
    return dict(result, status='diagnostic-' + status, detail=detail)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', type=Path, required=True)
    parser.add_argument('--shell', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--selection', choices=('es6id', 'es2015-features'), default='es6id',
                        help='Diagnostic candidate selection; never an edition decision')
    parser.add_argument('--filter', default='', help='Additional path substring restriction')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--timeout', type=float, default=15)
    args = parser.parse_args()
    args.suite, args.shell = args.suite.resolve(), args.shell.resolve()
    if args.jobs < 1 or args.timeout <= 0:
        parser.error('jobs and timeout must be positive')
    revision = subprocess.check_output(['git', '-C', str(args.suite), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != REVISION or subprocess.check_output(['git', '-C', str(args.suite), 'status', '--porcelain'], text=True):
        parser.error('Expected the clean pinned later checkout at ' + REVISION)
    cases = list(records(args.suite, args.filter, args.selection))
    print('Unreviewed later diagnostic modes:', len(cases), flush=True)
    before = historical.runtime_hashes(args.shell)
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(lambda case: run_case(case, args.suite, args.shell, args.timeout), cases):
            results.append(result)
            if len(results) % 500 == 0:
                print(len(results), 'complete', flush=True)
    counts = dict(collections.Counter(row['status'] for row in results))
    report = dict(purpose='Unreviewed metadata-selected diagnostic only; not full ES2015 conformance.',
                  revision=revision, selection=args.selection, filter=args.filter,
                  timezone='America/Los_Angeles', files=len({case['test'] for case in cases}),
                  counts=counts, results=results, runtime_sha256=before,
                  runtime_unchanged=before == historical.runtime_hashes(args.shell))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(counts), flush=True)
    return 0 if report['runtime_unchanged'] and counts.get('diagnostic-pass', 0) == len(cases) else 1


if __name__ == "__main__":
    raise SystemExit(main())
