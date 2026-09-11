#!/bin/sh

# Audit shipped PE files for x86 format, CRT DLL imports, and Windows 95/NT 4
# compatible PE version metadata.  A complete import listing is written for
# separate API review against both minimum operating systems.

set -eu

tree=${1-}
report=${2-}
if test -z "$tree" -o -z "$report"; then
    echo "usage: audit-pe.sh DIST-BIN IMPORT-REPORT" >&2
    exit 1
fi
if test ! -d "$tree"; then
    echo "audit-pe: not a directory: $tree" >&2
    exit 1
fi

if test -n "${PE_OBJDUMP-}"; then
    pe_objdump=$PE_OBJDUMP
elif command -v objdump >/dev/null 2>&1; then
    pe_objdump=objdump
elif command -v llvm-objdump >/dev/null 2>&1; then
    pe_objdump=llvm-objdump
else
    echo 'audit-pe: set PE_OBJDUMP to an objdump implementation with -p support' >&2
    exit 1
fi

tmp_base=${TMPDIR-/tmp}
tmp_dir=`mktemp -d "$tmp_base/zoolrunner-pe-audit.XXXXXX"` || exit 1
trap 'rm -rf "$tmp_dir"' 0 1 2 3 15
: > "$tmp_dir/imports"
: > "$tmp_dir/metadata"
: > "$tmp_dir/failures"

find -L "$tree" -type f \( -name '*.dll' -o -name '*.DLL' -o \
    -name '*.exe' -o -name '*.EXE' \) -print |
while IFS= read -r pe_file; do
    dump="$tmp_dir/dump"
    if ! "$pe_objdump" -p "$pe_file" > "$dump" 2>/dev/null; then
        echo "$pe_file: objdump could not read PE headers" >> "$tmp_dir/failures"
        continue
    fi

    if ! grep 'file format coff-i386' "$dump" >/dev/null 2>&1 ||
       ! grep 'Magic[[:space:]]*010b[[:space:]]*(PE32)' "$dump" >/dev/null 2>&1; then
        echo "$pe_file: is not an x86 PE32 image" >> "$tmp_dir/failures"
    fi

    awk -v file="$pe_file" '
        /DLL Name:/ { dll=$3; print file "\t" dll "\t"; in_imports=0; next }
        /Hint\/Ord[[:space:]]+Name/ { in_imports=1; next }
        in_imports && /^[[:space:]]*$/ { in_imports=0; next }
        in_imports && NF >= 2 { print file "\t" dll "\t" $NF }
    ' "$dump" >> "$tmp_dir/imports"

    if grep -Ei 'DLL Name: (MSVCR80|MSVCP80|MSVCR80D|MSVCP80D)\.dll' \
        "$dump" >/dev/null 2>&1; then
        echo "$pe_file: imports the Visual C++ 8 DLL runtime" >> "$tmp_dir/failures"
    fi

    os_major=`awk '/MajorOSystemVersion/{print $2; exit}' "$dump"`
    os_minor=`awk '/MinorOSystemVersion/{print $2; exit}' "$dump"`
    sub_major=`awk '/MajorSubsystemVersion/{print $2; exit}' "$dump"`
    sub_minor=`awk '/MinorSubsystemVersion/{print $2; exit}' "$dump"`
    subsystem=$(awk '/^Subsystem[[:space:]]/{print $2; exit}' "$dump")
    printf '%s\t%s\t%s\t%s\t%s\n' "$pe_file" "PE32-i386" \
        "${os_major-}.${os_minor-}" "${sub_major-}.${sub_minor-}" \
        "${subsystem-}" >> "$tmp_dir/metadata"
    if test -n "$os_major" -a -n "$os_minor"; then
        if test "$os_major" -gt 4 -o \
            \( "$os_major" -eq 4 -a "$os_minor" -gt 0 \); then
            echo "$pe_file: OS version is $os_major.$os_minor, later than 4.0" >> "$tmp_dir/failures"
        fi
    fi
    if test -n "$sub_major" -a -n "$sub_minor"; then
        if test "$sub_major" -gt 4 -o \
            \( "$sub_major" -eq 4 -a "$sub_minor" -gt 0 \); then
            echo "$pe_file: subsystem version is $sub_major.$sub_minor, later than 4.0" >> "$tmp_dir/failures"
        fi
    fi

    if command -v strings >/dev/null 2>&1 &&
       strings "$pe_file" 2>/dev/null |
       grep -Ei 'Microsoft\.VC80\.CRT|MSVC[PR]80D?\.dll' >/dev/null 2>&1; then
        echo "$pe_file: contains a VC80 CRT deployment reference" >> "$tmp_dir/failures"
    fi
done

LC_ALL=C sort -u "$tmp_dir/imports" > "$report"

# VC8's static CRT allocates a Win32 TLS slot in each image that contains it.
# Windows 95 and Windows NT 4 provide only 64 process TLS slots, so a build
# made from many /MT component DLLs can fail in the loader even though every
# DLL is otherwise valid.  Record the complete set and reject payloads that
# have already exhausted the architectural limit.
awk -F '\t' '
    toupper($2) == "KERNEL32.DLL" && $3 == "TlsAlloc" { print $1 }
' "$report" | LC_ALL=C sort -u > "$report.tls-slots.txt"
tls_slot_users=`wc -l < "$report.tls-slots.txt" | tr -d ' '`
if test "$tls_slot_users" -ge 64; then
    echo "$tree: $tls_slot_users PE images import TlsAlloc; Win95/NT4 provide only 64 process TLS slots" \
        >> "$tmp_dir/failures"
fi

# Verify that every non-system DLL named by the package is also present in the
# package.  This catches import-library identity mistakes (for example a .def
# file naming NSLIB.dll while the shipped LDAP library has another filename).
: > "$tmp_dir/packaged-dlls"
find -L "$tree" -type f \( -name '*.dll' -o -name '*.DLL' \) -print |
while IFS= read -r packaged_file; do
    basename "$packaged_file" | tr '[:lower:]' '[:upper:]'
done | LC_ALL=C sort -u > "$tmp_dir/packaged-dlls"
cut -f2 "$report" | sed '/^$/d' | tr '[:lower:]' '[:upper:]' |
    LC_ALL=C sort -u |
while IFS= read -r imported_dll; do
    case "$imported_dll" in
    ADVAPI32.DLL|COMCTL32.DLL|COMDLG32.DLL|GDI32.DLL|KERNEL32.DLL|\
    ODBC32.DLL|ODBCCP32.DLL|OLE32.DLL|OLEAUT32.DLL|RPCRT4.DLL|\
    SHELL32.DLL|USER32.DLL|UUID.DLL|VERSION.DLL|WINMM.DLL|\
    WINSPOOL.DRV|WS2_32.DLL|WSOCK32.DLL)
        ;;
    *)
        if ! grep -Fx "$imported_dll" "$tmp_dir/packaged-dlls" >/dev/null 2>&1; then
            echo "$tree: imported DLL is absent from package: $imported_dll" \
                >> "$tmp_dir/failures"
        fi
        ;;
    esac
done

{
    printf 'file\tformat\tos_version\tsubsystem_version\tsubsystem\n'
    LC_ALL=C sort -u "$tmp_dir/metadata"
} > "$report.metadata.tsv"

# Seed a platform-by-platform review without pretending that an unlisted API
# has been validated.  This deliberately emits "Requires investigation" for
# unknowns: PE 4.0 metadata and availability on one Windows family are not
# evidence for the other family.  Confirmed static-import blockers are called
# out so they cannot disappear in a large raw import inventory.
awk -F '\t' '
BEGIN {
    OFS="\t"
    print "file", "dll", "api", "windows_95", "windows_nt4_sp6a", "origin_or_note"
}
{
    dll=toupper($2)
    if (dll != "ADVAPI32.DLL" && dll != "COMDLG32.DLL" &&
        dll != "GDI32.DLL" && dll != "KERNEL32.DLL" &&
        dll != "OLE32.DLL" && dll != "OLEAUT32.DLL" &&
        dll != "RPCRT4.DLL" && dll != "SHELL32.DLL" &&
        dll != "USER32.DLL" && dll != "VERSION.DLL" &&
        dll != "WINMM.DLL" && dll != "WINSPOOL.DRV" &&
        dll != "WS2_32.DLL" && dll != "WSOCK32.DLL")
        next

    win95="Requires investigation"
    nt4="Requires investigation"
    note="unclassified static system import"

    if (dll == "KERNEL32.DLL" &&
        ($3 == "GetEnvironmentStringsW" ||
         $3 == "FreeEnvironmentStringsW" ||
         $3 == "GetStringTypeW" || $3 == "LCMapStringW")) {
        win95="Incompatible"
        note="introduced by the MSVC 8 static CRT; direct Unicode import"
    } else if (dll == "KERNEL32.DLL" && $3 == "IsDebuggerPresent") {
        win95="Incompatible"
        note="introduced by the MSVC 8 static CRT; absent from Windows 95"
    } else if ($3 ~ /W$/) {
        win95="Requires optional Unicode layer or source fallback"
        note="Unicode entry point: verify export or historical ANSI fallback"
    }

    print $1, $2, $3, win95, nt4, note
}
' "$report" > "$report.compatibility.tsv"

if test -s "$tmp_dir/failures"; then
    cat "$tmp_dir/failures" >&2
    exit 1
fi

pe_count=`cut -f1 "$report" | LC_ALL=C sort -u | wc -l | tr -d ' '`
echo "PE audit passed for $pe_count files; imports written to $report"
echo "PE metadata written to $report.metadata.tsv"
echo "Win95/NT4 review seed written to $report.compatibility.tsv"
echo "TLS-slot users: $tls_slot_users; list written to $report.tls-slots.txt"
