#!/usr/bin/env python3
"""Build the host dependency scanner and check bundled PNG header discovery."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true', help='Enable host ASan/UBSan')
args = parser.parse_args()
source = Path(__file__).resolve().parent
root = source.parents[1]
png = root / 'modules/libimg/png'
zlib = root / 'modules/zlib/src'
compiler = shlex.split(os.environ.get('HOST_CC', 'cc'))
flags = ['-std=gnu89', '-O2', '-DNO_X11', '-DXP_UNIX', '-DINCLUDEDIR="/usr/include"', '-DOBJSUFFIX=".obj"']
if args.sanitize:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
env = dict(os.environ)
# The historical command-line tool leaves its process-lifetime tables allocated.
if args.sanitize:
    env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=0'
    env['UBSAN_OPTIONS'] = env.get('UBSAN_OPTIONS', '') + ':halt_on_error=1'
with tempfile.TemporaryDirectory(prefix='zool-mkdepend-') as directory:
    work = Path(directory)
    scanner = work / 'mkdepend'
    sources = ['cppsetup.c', 'ifparser.c', 'include.c', 'main.c', 'parse.c', 'pr.c']
    subprocess.run(compiler + flags + [str(source / f) for f in sources] +
                   ['-o', str(scanner)], check=True, env=env)

    def scan(path, options):
        depfile = work / 'dependencies.mk'
        depfile.write_text('')
        result = subprocess.run([str(scanner), '-f' + str(depfile), '-o.obj'] +
                                options + [str(path)], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode:
            raise RuntimeError(str(path) + '\n' + result.stdout + result.stderr)
        return depfile.read_text()

    # Verify that nested object-like macro evaluation still selects headers.
    (work / 'selected.h').write_text('/* selected dependency */\n')
    (work / 'unselected.h').write_text('/* inactive branch */\n')
    fixture = work / 'conditional.c'
    fixture.write_text('#define BASE (1 + 1)\n#define VALUE (BASE + 1)\n'
                       '#if VALUE == 3\n#include "selected.h"\n'
                       '#else\n#include "unselected.h"\n#endif\n')
    deps = scan(fixture, ['-I' + str(work)])
    assert 'selected.h' in deps and 'unselected.h' not in deps, deps

    # Use production source: its valid chunk-check macros exercise diagnostics
    # for syntax beyond this historical scanner's partial preprocessor grammar.
    options = ['-DMOZ_PNG_READ', '-DMOZ_PNG_WRITE', '-DZLIB_INTERNAL',
               '-I' + str(png), '-I' + str(zlib)]
    for arch in ['ARM_NEON', 'INTEL_SSE', 'LOONGARCH_LSX', 'MIPS_MSA',
                 'POWERPC_VSX', 'RISCV_RVV']:
        options.append('-DPNG_' + arch + '_OPT=0')
    deps = scan(png / 'png.c', options)
    for header in ['pngpriv.h', 'png.h', 'pngconf.h', 'pnglibconf.h',
                   'pngstruct.h', 'pnginfo.h', 'zlib.h', 'zconf.h']:
        assert header in deps, 'Missing PNG dependency: ' + header
print('PASS: nested conditional headers and bundled PNG dependency scan')
