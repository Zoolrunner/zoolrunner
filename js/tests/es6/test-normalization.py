#!/usr/bin/env python3
"""Run Unicode 18.0.0 normalization invariants through the actual JS methods."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

DIGEST = '25a50d816764b04abfb4a646d3eb2b2a803284c3873d9a06757b94fe4513dde3'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--shell', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    data = args.data.read_bytes()
    if hashlib.sha256(data).hexdigest() != DIGEST:
        parser.error('NormalizationTest.txt does not match pinned Unicode 18.0.0')
    rows, listed, part = [], set(), None
    for line in data.decode('utf-8').splitlines():
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        if line.startswith('@'):
            part = line
            continue
        fields = line.split(';')[:5]
        points = [[int(value, 16) for value in field.split()] for field in fields]
        rows.append([''.join(chr(value) for value in field) for field in points])
        if part == '@Part1':
            if len(points[0]) != 1:
                raise ValueError('Unexpected multi-code-point Part 1 entry')
            listed.add(points[0][0])
    shell = args.shell.absolute()  # Preserve dist/bin loader paths through links.
    environment = dict(os.environ)
    environment['DYLD_LIBRARY_PATH'] = str(shell.parent)
    environment['LD_LIBRARY_PATH'] = str(shell.parent)
    start = time.monotonic()
    with tempfile.TemporaryDirectory(prefix='zool-unicode-normalization-') as temporary:
        base = Path(temporary)
        chunks = []
        for offset in range(0, len(rows), 256):
            path = base / ('rows-%d.js' % offset)
            path.write_text('verifyRows(' + json.dumps(rows[offset:offset+256], ensure_ascii=True) + ');\n', encoding='ascii')
            chunks.append(str(path))
        driver = base / 'driver.js'
        source = r'''
var checks=0, rows=0, identity=0;
var forms=['NFC','NFD','NFKC','NFKD'];
var expected=[[1,1,1,3,3],[2,2,2,4,4],[3,3,3,3,3],[4,4,4,4,4]];
function verifyRows(data) {
 for(var row=0;row<data.length;++row,++rows) {
  for(var form=0;form<4;++form) {
   for(var column=0;column<5;++column) {
    ++checks;
    if(data[row][column].normalize(forms[form])!==data[row][expected[form][column]])
     throw Error('Unicode normalization row '+rows+' '+forms[form]+' column '+column);
   }
  }
 }
}
var chunks=CHUNKS;
for(var i=0;i<chunks.length;++i)load(chunks[i]);
var listed=LISTED, excluded={}, point, form, str;
for(i=0;i<listed.length;++i)excluded[listed[i]]=true;
for(point=0;point<=0x10ffff;++point) {
 if(excluded[point])continue;
 str=String.fromCodePoint(point);
 for(form=0;form<4;++form) {
  ++checks;
  if(str.normalize(forms[form])!==str)
   throw Error('Unicode normalization identity '+point+' '+forms[form]);
 }
 ++identity;
 if(point%65536===0)print('UNICODE-PROGRESS codepoint='+point);
}
print('UNICODE-NORMALIZATION rows='+rows+' identity='+identity+' checks='+checks+' failures=0');
'''.replace('CHUNKS', json.dumps(chunks)).replace('LISTED', json.dumps(sorted(listed)))
        # ASCII escapes preserve every UTF-16 code unit through historical load().
        driver.write_text(source, encoding='ascii')
        result = subprocess.run([str(shell), '-E', '-f', str(driver)], env=environment,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=300)
    identity = 0x110000 - len(listed)
    checks = len(rows) * 20 + identity * 4
    marker = 'UNICODE-NORMALIZATION rows=%d identity=%d checks=%d failures=0' % (len(rows), identity, checks)
    passed = result.returncode == 0 and marker in result.stdout
    report = {'unicode_version': '18.0.0', 'data_sha256': DIGEST,
              'shell': str(shell), 'rows': len(rows), 'identity_code_points': identity,
              'checks': checks, 'pass': passed, 'seconds': time.monotonic()-start,
              'output': result.stdout}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(result.stdout, end='')
    if not passed:
        raise SystemExit('Unicode normalization conformance failed')


if __name__ == '__main__':
    main()
