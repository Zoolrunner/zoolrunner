#!/bin/sh
# Build the standard-library configuration used by the early Darwin applications.
set -eu
if test "$#" != 3; then
  echo "Usage: $0 GCC_SOURCE_ARCHIVE GCC4_HEADER_DIRECTORY NEW_OUTPUT_DIRECTORY" >&2
  exit 1
fi
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the original-library overlay}"
: "${ZR_PPC_EARLY_LINKER:?Set ZR_PPC_EARLY_LINKER to the classic 10.0 linker}"
ZR_SCRIPTS=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
zr_archive=$1
sh "$ZR_SCRIPTS/build-early-cxx-runtime.sh" "$1" "$2" "$3"
ZR_STDLIB_OUTPUT=`CDPATH= cd -- "$3" && pwd`
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
tar -xzf "$zr_archive" -C "$zr_work" gcc-5247/libstdc++-v3
ZR_STDLIB_SOURCE="$zr_work/gcc-5247/libstdc++-v3"
python3 - "$ZR_STDLIB_OUTPUT/include" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
path = root / 'powerpc-apple-darwin8/bits/c++config.h'
text = path.read_text()
# These are actual absent early-OS facilities, not application feature switches.
# The resulting library does not promise the later GCC wide-stream interfaces.
for macro in ('_GLIBCXX_USE_WCHAR_T', '_GLIBCXX_HAVE_WCHAR_H',
              '_GLIBCXX_HAVE_WCTYPE_H', '_GLIBCXX_HAVE_MBSTATE_T',
              '_GLIBCXX_HAVE_POLL', '_GLIBCXX_HAVE_POLL_H',
              '_GLIBCXX_HAVE_LC_MESSAGES', '_GLIBCXX_HAVE_STRTOF',
              '_GLIBCXX_HAVE_STRTOLD'):
    text = text.replace('#define ' + macro + ' 1',
                        '/* ' + macro + ' unavailable in early Darwin */')
path.write_text(text)
path = root / 'cctype'
text = path.read_text()
old = '#include <ctype.h>'
if text.count(old) != 1:
    raise SystemExit('Unexpected cctype header revision')
names = ('isalnum', 'isalpha', 'iscntrl', 'isdigit', 'isgraph', 'islower',
         'isprint', 'ispunct', 'isspace', 'isupper', 'isxdigit')
declarations = '\nextern "C" {\n' + ''.join('int (' + n + ')(int);\n' for n in names) + '}\n'
path.write_text(text.replace(old, old + declarations))
PY
export ZR_MACOS_SDK ZR_PPC_EARLY_LINKER ZR_STDLIB_OUTPUT ZR_STDLIB_SOURCE ZR_SCRIPTS
mkdir "$zr_work/obj"
cd "$zr_work/obj"
make -f "$ZR_SCRIPTS/early-stdlib.mk" -j"${ZR_BUILD_JOBS:-4}"
cp libzoolcxx.dylib libgcc-10.0.a "$ZR_STDLIB_OUTPUT/"
echo "Built early Darwin C++ library; wide-character streams are not supplied by this configuration"
