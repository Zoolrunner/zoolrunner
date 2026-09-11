#!/bin/sh

set -eu

script_dir=`CDPATH= cd "$(dirname "$0")" && pwd` || exit 1

if test -z "${MSVC8_ROOT-}"; then
    printf '%s\n' 'test-toolchain: MSVC8_ROOT is not set' >&2
    exit 1
fi

tmp_base=${TMPDIR-/tmp}
test_dir=`mktemp -d "$tmp_base/zoolrunner-msvc8-test.XXXXXX"` || exit 1
trap 'rm -rf "$test_dir"' 0 1 2 3 15
work_dir="$test_dir/path with spaces"
mkdir "$work_dir"

cat > "$work_dir/hello.c" <<'EOF'
#include <stdio.h>

int main(void)
{
    puts("hello from MSVC 8 under Wine");
    return 0;
}
EOF

printf '%s\n' /nologo /TC /MT /c \
    "\"/Fo$work_dir/hello.obj\"" "\"$work_dir/hello.c\"" \
    > "$test_dir/compile.rsp"
printf '%s\n' /nologo "\"/OUT:$work_dir/hello.exe\"" \
    "\"$work_dir/hello.obj\"" > "$test_dir/link.rsp"

"$script_dir/cl.sh" "@$test_dir/compile.rsp"
"$script_dir/link.sh" "@$test_dir/link.rsp"

if command -v file >/dev/null 2>&1; then
    file "$work_dir/hello.obj" "$work_dir/hello.exe"
    if ! file "$work_dir/hello.exe" | grep 'PE32 executable' >/dev/null 2>&1; then
        printf '%s\n' 'test-toolchain: output is not a PE32 executable' >&2
        exit 1
    fi
fi

if command -v objdump >/dev/null 2>&1; then
    imports=`objdump -p "$work_dir/hello.exe" 2>/dev/null || :`
    if printf '%s\n' "$imports" |
       grep -Ei 'DLL Name: (MSVCR80|MSVCP80|MSVCR80D|MSVCP80D)\.dll' >/dev/null 2>&1; then
        printf '%s\n' 'test-toolchain: output imports the Visual C++ 8 DLL runtime' >&2
        exit 1
    fi
fi

exe_win=`"$script_dir/wine-run.sh" --winepath "$work_dir/hello.exe"`
"$script_dir/wine-run.sh" -- "$exe_win"

printf '%s\n' 'MSVC 8 standalone compile, link, format, and execution test passed.'
