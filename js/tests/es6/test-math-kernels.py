#!/usr/bin/env python3
"""Compare the bundled numeric kernels with a modern host's C99 libm.

This is a host diagnostic, not target-OS application runtime validation.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objdir', type=Path, required=True,
                        help='configured object directory with js/src/jsautocfg.h')
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    configured = args.objdir.absolute() / 'js/src'
    if not (configured / 'jsautocfg.h').is_file():
        parser.error('the object directory must contain generated jsautocfg.h')
    names = ['s_expm1', 's_log1p', 's_cbrt', 's_asinh', 's_tanh',
             'e_acosh', 'e_atanh', 'e_cosh', 'e_sinh', 'e_log10']
    with tempfile.TemporaryDirectory(prefix='zool-math-kernels-') as temporary:
        executable = Path(temporary) / 'math-kernels'
        command = shlex.split(args.cc) + [
            '-std=c99', '-O2', '-DXP_UNIX', '-DZR_ES2015_FDLIBM',
            '-I' + str(root / 'js/src'), '-I' + str(configured)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        command += [str(root / 'js/tests/es6/TestMathKernels.c')]
        command += [str(root / 'js/src/fdlibm' / (name + '.c')) for name in names]
        command += ['-lm', '-o', str(executable)]
        subprocess.run(command, check=True, timeout=120)
        result = subprocess.run([str(executable)], check=True, timeout=120,
                                stdout=subprocess.PIPE, text=True)
        print(result.stdout, end='')
        if 'MATH-PORT checks=2000510 failures=0' not in result.stdout:
            raise RuntimeError('Numeric probe did not finish all comparisons')


if __name__ == '__main__':
    main()
