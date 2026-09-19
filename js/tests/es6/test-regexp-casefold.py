#!/usr/bin/env python3
"""Check every pinned simple/common case-fold mapping in Unicode regexps."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

DIGEST = 'a004797658a457bec4dc11683e39f69249ea3b595b752dbea6721c4c9f587b0d'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ucd', type=Path, required=True)
    parser.add_argument('--shell', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    args = parser.parse_args()
    data = (args.ucd / 'CaseFolding.txt').read_bytes()
    if hashlib.sha256(data).hexdigest() != DIGEST:
        raise ValueError('Unicode case-fold input checksum mismatch')
    rows = []
    for line in data.decode('utf-8').splitlines():
        fields = [s.strip() for s in line.split('#', 1)[0].split(';')]
        if len(fields) >= 3 and fields[1] in ('C', 'S'):
            rows.append([int(fields[0], 16), int(fields[2], 16)])
    assert len(rows) == 1533
    code = 'var rows=' + json.dumps(rows) + ';\n' + r'''
var checks=0;
function check(ok, row, kind) {
    checks++;
    if(!ok)throw Error('fold '+row+' '+kind);
}
rows.forEach(function(row){
    for(var direction=0;direction<2;direction++) {
        var from=row[direction], to=row[1-direction];
        var escaped='\\u{'+from.toString(16)+'}', text=String.fromCodePoint(to);
        check(new RegExp('^'+escaped+'$','iu').test(text),row,'literal');
        check(new RegExp('^['+escaped+']$','iu').test(text),row,'class');
        check(!new RegExp('^[^'+escaped+']$','iu').test(text),row,'negative class');
        check(new RegExp('^('+escaped+')\\1$','iu').test(String.fromCodePoint(from)+text),row,'backref');
        check(!new RegExp('^'+escaped+'$','u').test(text),row,'case-sensitive');
    }
});
print('UNICODE-REGEXP-FOLD checks='+checks+' failures=0');
'''
    shell = args.shell.absolute()
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent), LD_LIBRARY_PATH=str(shell.parent))
    with tempfile.TemporaryDirectory(prefix='zr-regexp-fold-') as temp:
        script = Path(temp) / 'fold.js'
        script.write_text(code, encoding='ascii')
        with args.log.open('w') as log:
            result = subprocess.run([str(shell), '-E', '-v', '2015', '-f', str(script)], env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600)
    marker = 'UNICODE-REGEXP-FOLD checks=%d failures=0' % (10 * len(rows))
    if result.returncode or marker not in args.log.read_text(errors='replace'):
        raise SystemExit('Regexp mapping checks failed; see ' + str(args.log))
    print(marker)

if __name__ == '__main__':
    main()
