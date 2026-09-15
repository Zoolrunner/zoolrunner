#!/bin/sh
# Rebuild original startup objects for the experimental PowerPC targets.
set -eu
zr_script_dir=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the target SDK or experimental overlay}"
zr_target=${ZR_PPC_DEPLOYMENT_TARGET:-10.3.9}
zr_prefix=/opt/mac/bin/powerpc-apple-darwin8-
case "$zr_target" in
  10.3.9)
    zr_revision=c13a3023d5a0a8b8bf6b4fcc56f741d5ff789b4b
    zr_checksum=64925c3d31f6aececdd6b2fa7b00056ea6ca6f33884e7f084b70e1dba41036dd
    zr_linker=${zr_prefix}ld_classic
    zr_destination=/opt/mac/lib/zoolrunner-panther
    ;;
  10.0)
    zr_revision=809753418c8044b256903fa84f03e69f1b5531a7
    zr_checksum=764a316fbbd61997851e8e8d8266fdacf2d97440de72e56c08f96d2d4e7fa0f1
    : "${ZR_PPC_EARLY_LINKER:?Set ZR_PPC_EARLY_LINKER to the rebuilt 10.0 linker}"
    zr_linker=$ZR_PPC_EARLY_LINKER
    zr_destination=/opt/mac/lib/zoolrunner-cheetah
    ;;
  *) echo "Unsupported startup target: $zr_target" >&2; exit 1 ;;
esac
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
curl --fail --location --retry 3 --output "$zr_work/csu.tar.gz" \
  "https://codeload.github.com/apple-oss-distributions/Csu/tar.gz/$zr_revision"
printf '%s  %s\n' "$zr_checksum" \
  "$zr_work/csu.tar.gz" | sha256sum -c -
tar -xzf "$zr_work/csu.tar.gz" -C "$zr_work" --strip-components=1
cd "$zr_work"
for zr_source in start dyld; do
  "${zr_prefix}gcc" -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min="$zr_target" \
    -mcpu=G3 -mno-altivec -dynamic -DCRT1 -x assembler-with-cpp \
    -c "$zr_source.s" -o "$zr_source.o"
done
set --
if test "$zr_target" = 10.0; then
  set -- -D__keymgr_dwarf2_register_sections=zr_initialize_gcc_unwind
fi
"${zr_prefix}gcc" -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min="$zr_target" \
  -mcpu=G3 -mno-altivec -mlong-double-64 -dynamic -DCRT1 -fno-builtin \
  "$@" -c crt.c -o crt.o
set --
if test "$zr_target" = 10.0; then
  "${zr_prefix}gcc" -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min=10.0 \
    -mcpu=G3 -mno-altivec -mlong-double-64 -dynamic \
    -c "$zr_script_dir/gcc-unwind-bridge.c" -o gcc-unwind-bridge.o
  set -- gcc-unwind-bridge.o /opt/mac/lib/gcc/powerpc-apple-darwin8/4.0.1/crt2.o
fi
# Csu's original link uses the installed dyld to record /usr/lib/dyld.
# Linux has no such executable. This temporary link stub supplies that name
# only; its code is never copied into crt1.o or a shipped application.
cat > dyld-link-stub.s <<'ASM'
.machine ppc750
.text
.globl __dyld_start
__dyld_start:
    blr
ASM
"${zr_prefix}as" -arch ppc dyld-link-stub.s -o dyld-link-stub.o
"${zr_prefix}ld" -dylinker -dylinker_install_name /usr/lib/dyld \
  -e __dyld_start -o dyld-link-stub dyld-link-stub.o
# The classic linker retains the required loader/entry commands in an object
# when explicitly targeting these old systems. ld64 does not support this.
"$zr_linker" -arch ppc -macosx_version_min "$zr_target" \
  -r -dynamic -keep_private_externs dyld-link-stub start.o crt.o dyld.o \
  "$@" -o icrt1.o
"${zr_prefix}indr" -arch all indr_list icrt1.o crt1.o
"${zr_prefix}otool" -l crt1.o > load-commands.txt
grep 'LC_LOAD_DYLINKER' load-commands.txt >/dev/null
grep 'LC_UNIXTHREAD' load-commands.txt >/dev/null
mkdir -p "$zr_destination"
cp crt1.o "$zr_destination/crt1.o"
# Shared libraries and bundles also need their matching dyld binding helpers.
# Build these from the same Csu revision instead of borrowing newer SDK objects.
for zr_source in dylib bundle1; do
  "${zr_prefix}gcc" -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min="$zr_target" \
    -mcpu=G3 -mno-altivec -dynamic -x assembler-with-cpp \
    -c "$zr_source.s" -o "$zr_source.o"
done
"${zr_prefix}gcc" -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min="$zr_target" \
  -mcpu=G3 -mno-altivec -mlong-double-64 -dynamic -c icplusplus.c -o icplusplus.o
"$zr_linker" -arch ppc -macosx_version_min "$zr_target" \
  -r -dynamic -keep_private_externs dylib.o icplusplus.o -o dylib1.o
cp dylib1.o bundle1.o "$zr_destination/"
