#!/usr/bin/env python3
"""Check extended immutable-write operands and their decompilation across the 16-bit atom boundary."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--shell', type=Path, required=True)
args = parser.parse_args()
shell = args.shell.absolute()
environment = dict(os.environ, DYLD_LIBRARY_PATH=str(shell.parent),
                   LD_LIBRARY_PATH=str(shell.parent))
literals = ''.join('value=' + json.dumps('constant-atom-' + str(i)) + ';'
                   for i in range(65540))
source = ('(function largeConst(runBody){const fixed=1;var value;if(runBody){' +
          literals + '}WRITE;})')
driver = '''var source=SOURCE, checks=0, decompiled='';
function verify(fn, label) {
    var caught=false;
    try { fn(false); } catch(error) { caught=error instanceof TypeError && /constant binding/.test(error.message); }
    if(!caught) throw Error(label+': '+decompiled.slice(-250));
    checks++;
}
var writes=['fixed=2', 'fixed+=2', '[fixed]=[2]',
            '[fixed]=({0:2,length:1})', '({value:fixed}={value:2})'];
for (var i=0;i<writes.length;i++) {
    var fn=evaluate(source.replace('WRITE',writes[i]),'extended-const-operand');
    verify(fn, 'compiled '+writes[i]);
    decompiled=fn.toString();
    verify(evaluate('('+decompiled+')','extended-const-decompile'),
           'decompiled '+writes[i]);
}
function verifyObject(fn) {
    var object=fn(false);
    if (object.length!==1 || object[0]!==2 || object.value!==3 ||
        Object.getOwnPropertyNames(object).length!==3)
        throw Error('extended object property/value ordering');
    checks++;
}
for (var edition=0;edition<2;edition++) {
    version(edition ? 180 : 2015);
    var objectSource=source.replace('WRITE', 'return {0:2,length:1,value:3}');
    var objectFn=evaluate(objectSource,'extended-object-properties');
    verifyObject(objectFn);
    verifyObject(evaluate('('+objectFn.toString()+')','extended-object-decompile'));
}
print('ES6-CONST-LARGE-SCRIPT checks='+checks+' failures=0');
'''.replace('SOURCE', json.dumps(source))
with tempfile.TemporaryDirectory(prefix='zool-const-large-') as temporary:
    path = Path(temporary) / 'driver.js'
    path.write_text(driver)
    result = subprocess.run([str(shell), '-E', '-f', str(path)], env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=120)
    print(result.stdout, end='')
    if result.returncode or 'ES6-CONST-LARGE-SCRIPT checks=14 failures=0' not in result.stdout:
        raise SystemExit('Extended const operand regression failed')
