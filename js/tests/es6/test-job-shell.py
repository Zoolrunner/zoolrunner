#!/usr/bin/env python3
"""Check shell job checkpoints, nested evaluation and failure exit statuses."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--shell', required=True, type=Path)
parser.add_argument('--standalone', action='store_true')
args = parser.parse_args()
shell = args.shell.absolute()
env = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent),
           LD_LIBRARY_PATH=str(shell.parent), MallocScribble='1')
base = [str(shell)] + ([] if args.standalone else ['-E']) + ['-v', '2015']
checks = 0


def run(arguments, expected, status=0, stdin=None):
    global checks
    result = subprocess.run(base + arguments, input=stdin, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=20)
    lines = [line for line in result.stdout.splitlines() if line.startswith('JOB:')]
    checks += 1
    if result.returncode != status or lines != expected:
        raise RuntimeError((arguments, status, expected, result.returncode,
                            result.stdout, result.stderr))


with tempfile.TemporaryDirectory(prefix='zoolrunner-job-shell-') as temporary:
    root = Path(temporary)
    nested = root / 'nested.js'
    nested.write_text('enqueueJob(function(){gc();print("JOB:nested")});')
    source = ('enqueueJob(function(){print("JOB:first");'
              'enqueueJob(function(){print("JOB:last")});'
              'drainJobQueue();print("JOB:still-first")});'
              'load(' + json.dumps(str(nested)) + ');'
              'print("JOB:script");gc();')
    expected = ['JOB:script', 'JOB:first', 'JOB:still-first', 'JOB:nested', 'JOB:last']
    run(['-e', source], expected)
    main = root / 'main.js'
    main.write_text(source)
    run(['-f', str(main)], expected)
    run([], expected, stdin=source)
    run(['-e', source, '-e', 'print("JOB:next-turn")'], expected + ['JOB:next-turn'])
    run(['-C', '-f', str(main)], [])
    run(['-e', 'enqueueJob(function(){throw Error("job failure")});'], [], status=3)
    run(['-e', 'enqueueJob(function(){quit(0)});'
                'enqueueJob(function(){print("JOB:after-quit")});'], [])
    run(['-e', 'var index=0;for(var i=0;i<2048;i++){(function(n){'
                'enqueueJob(function(){if(n!==index++)throw Error("FIFO");'
                'if(n%127===0)gc();})})(i);if(i%127===0)gc();}'
                'enqueueJob(function(){print("JOB:burst="+index)});'],
        ['JOB:burst=2048'])
    run(['-e', 'enqueueJob(function(){throw Error("job failure")});'
                'enqueueJob(function(){print("JOB:later")});'
                'try {drainJobQueue()} catch(e) {print("JOB:caught")}'
                'drainJobQueue();'], ['JOB:caught', 'JOB:later'])
    if not args.standalone:
        evaluated = 'enqueueJob(function(){print("JOB:evaluated")})'
        run(['-e', 'evaluate(' + json.dumps(evaluated) + ');print("JOB:outer")'],
            ['JOB:outer', 'JOB:evaluated'])
print('ES6-JOB-SHELL checks=%d failures=0' % checks)
