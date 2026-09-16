#!/bin/sh
# Compare the host-generated SpiderMonkey ABI header with actual target types.
set -eu
zr_root=${1:?source root required}
zr_obj=${2:?object directory required}
zr_tmp=`mktemp -d`
trap 'rm -rf "$zr_tmp"' 0 1 2 3 15
cat > "$zr_tmp/probe.c" <<'C'
#include <stddef.h>
#include "jsautocfg.h"
#define CHECK(name, condition) typedef char name[(condition) ? 1 : -1]
#define ALIGN(type) offsetof(struct { char c; type value; }, value)
CHECK(word, JS_BYTES_PER_WORD == sizeof(void *));
CHECK(long_size, JS_BYTES_PER_LONG == sizeof(long));
CHECK(double_size, JS_BYTES_PER_DOUBLE == sizeof(double));
CHECK(int64_size, JS_BYTES_PER_INT64 == sizeof(long long));
CHECK(pointer_align, JS_ALIGN_OF_POINTER == ALIGN(void *));
CHECK(double_align, JS_ALIGN_OF_DOUBLE == ALIGN(double));
CHECK(int64_align, JS_ALIGN_OF_INT64 == ALIGN(long long));
#ifndef IS_LITTLE_ENDIAN
#error Linux x86 must be little endian
#endif
int main(void) { return 0; }
C
for zr_bits in 32 64; do
  gcc -DCROSS_COMPILE -DXP_UNIX '-DMDCPUCFG="md/_linux.cfg"' \
    -DJS_TARGET_LINUX_X86_BITS="$zr_bits" -I"$zr_obj/dist/include/nspr" \
    "$zr_root/js/src/jscpucfg.c" -o "$zr_tmp/generator"
  "$zr_tmp/generator" > "$zr_tmp/jsautocfg.h"
  gcc -m"$zr_bits" "$zr_tmp/probe.c" -o "$zr_tmp/probe"
  "$zr_tmp/probe"
  echo "PASS: Linux x86 $zr_bits-bit generated ABI"
done
