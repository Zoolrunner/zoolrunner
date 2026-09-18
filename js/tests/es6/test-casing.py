#!/usr/bin/env python3
"""Check modern casing for every Unicode code point against pinned UCD inputs."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('casing_generator', ROOT / 'js/src/unicode/generate-casing.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ucd', type=Path, required=True)
    parser.add_argument('--shell', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    args = parser.parse_args()
    texts = {}
    for name, digest in module.HASHES.items():
        data = (args.ucd / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('Unicode input checksum mismatch: ' + name)
        texts[name] = data.decode('utf-8')
    lower, upper = {}, {}
    for line in texts['UnicodeData.txt'].splitlines():
        fields = line.split(';')
        point = int(fields[0], 16)
        if fields[12]: upper[point] = chr(int(fields[12], 16))
        if fields[13]: lower[point] = chr(int(fields[13], 16))
    for line in texts['SpecialCasing.txt'].splitlines():
        fields = line.split('#', 1)[0].split(';')
        if len(fields) < 5 or fields[4].strip(): continue
        point = int(fields[0], 16)
        lower[point] = ''.join(chr(int(v, 16)) for v in fields[1].split())
        upper[point] = ''.join(chr(int(v, 16)) for v in fields[3].split())
    code = 'var lower=' + json.dumps(lower, ensure_ascii=True) + ';\nvar upper=' + json.dumps(upper, ensure_ascii=True) + ';\n'
    code += r'''
var checks = 0;
for (var point = 0; point <= 0x10ffff; ++point) {
    var text = String.fromCodePoint(point);
    var lo = lower.hasOwnProperty(point) ? lower[point] : text;
    var up = upper.hasOwnProperty(point) ? upper[point] : text;
    var actual = [text.toLowerCase(), text.toUpperCase(), text.toLocaleLowerCase(), text.toLocaleUpperCase()];
    var expected = [lo, up, lo, up];
    for (var method = 0; method < 4; ++method) {
        ++checks;
        if (actual[method] !== expected[method]) throw new Error('casing U+' + point.toString(16) + ' method ' + method);
    }
}
print('UNICODE-CASING checks=' + checks + ' failures=0');
'''
    shell = args.shell.absolute()  # Keep the runtime directory of a symlinked shell.
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent), MallocScribble='1')
    with tempfile.TemporaryDirectory(prefix='zr-casing-') as temp:
        script = Path(temp) / 'casing.js'
        script.write_text(code, encoding='ascii')
        with args.log.open('w') as log:
            result = subprocess.run([str(shell), '-E', '-f', str(script)], env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600)
    marker = 'UNICODE-CASING checks=4456448 failures=0'
    if result.returncode or marker not in args.log.read_text(errors='replace'):
        raise SystemExit('Unicode casing failed; see ' + str(args.log))
    print(marker)

if __name__ == '__main__': main()
