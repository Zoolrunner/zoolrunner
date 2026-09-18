#!/usr/bin/env python3
"""Exercise identifier property boundaries using pinned UCD source data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

DIGEST = '09c928886a178fcafd93c29e4bd59073a058e5a100b716d425cb563ab50f68c9'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ucd', required=True, type=Path)
    parser.add_argument('--shell', required=True, type=Path)
    parser.add_argument('--log', required=True, type=Path)
    args = parser.parse_args()
    data = (args.ucd / 'DerivedCoreProperties.txt').read_bytes()
    if hashlib.sha256(data).hexdigest() != DIGEST:
        raise ValueError('Unicode input checksum mismatch')
    properties = {'ID_Start': set(), 'ID_Continue': set()}
    points = set(range(128)) | {0x200c, 0x200d, 0xd800, 0xdc00, 0x10ffff}
    for line in data.decode('utf-8').splitlines():
        fields = [v.strip() for v in line.split('#', 1)[0].split(';')]
        if len(fields) < 2 or fields[1] not in properties:
            continue
        ends = fields[0].split('..')
        lo, hi = int(ends[0], 16), int(ends[-1], 16)
        properties[fields[1]].update(range(lo, hi + 1))
        points.update(p for p in (lo-1, lo, hi, hi+1) if 0 <= p <= 0x10ffff)
    properties['ID_Start'].update((0x24, 0x5f))
    properties['ID_Continue'].update((0x24, 0x5f, 0x200c, 0x200d))
    rows = [[p, p in properties['ID_Start'], p in properties['ID_Continue']] for p in sorted(points)]
    code = 'var rows=' + json.dumps(rows) + ';\n' + r'''
var checks=0;
function accepts(source, name) {
    try {return Function('return function '+source+'(){}')().name===name;}
    catch(e) {if(e instanceof SyntaxError)return false;throw e;}
}
rows.forEach(function(row){
    var p=row[0], raw=String.fromCodePoint(p), escaped='\\u{'+p.toString(16)+'}';
    [raw,escaped].forEach(function(text){
        [true,false].forEach(function(start){
            var prefix=start?'':'Q', suffix='Q', expected=row[start?1:2];
            checks++;
            if(accepts(prefix+text+suffix,prefix+raw+suffix)!==expected)
                throw Error('identifier U+'+p.toString(16)+' start='+start+' escaped='+(text===escaped));
        });
    });
});
print('UNICODE-IDENTIFIERS checks='+checks+' failures=0');
'''
    shell = args.shell.absolute()
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent), LD_LIBRARY_PATH=str(shell.parent), MallocScribble='1')
    with tempfile.TemporaryDirectory(prefix='zr-identifiers-') as temp:
        script = Path(temp) / 'identifiers.js'
        script.write_text(code, encoding='ascii')
        with args.log.open('w') as log:
            result = subprocess.run([str(shell), '-E', '-v', '2015', '-f', str(script)], env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600)
    marker = 'UNICODE-IDENTIFIERS checks=%d failures=0' % (4 * len(rows))
    if result.returncode or marker not in args.log.read_text(errors='replace'):
        raise SystemExit('Identifier property checks failed; see ' + str(args.log))
    print(marker)

if __name__ == '__main__':
    main()
