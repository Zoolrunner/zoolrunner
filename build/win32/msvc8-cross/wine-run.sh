#!/bin/sh

# Locate and invoke a Wine-compatible runtime.  This file intentionally uses
# only POSIX shell syntax because it is shared by macOS/CrossOver and Unix/Wine
# build hosts.

set -eu

find_wine()
{
    if test -n "${MSVC8_WINE-}"; then
        printf '%s\n' "$MSVC8_WINE"
        return
    fi
    if test -n "${WINE-}"; then
        printf '%s\n' "$WINE"
        return
    fi
    if command -v wine >/dev/null 2>&1; then
        command -v wine
        return
    fi

    case `uname -s 2>/dev/null || printf unknown` in
    Darwin)
        crossover_wine=/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine
        if test -x "$crossover_wine"; then
            printf '%s\n' "$crossover_wine"
            return
        fi
        ;;
    esac

    printf '%s\n' 'msvc8-cross: could not find Wine; set MSVC8_WINE or WINE' >&2
    exit 1
}

wine=`find_wine`
if test ! -x "$wine"; then
    if ! command -v "$wine" >/dev/null 2>&1; then
        printf "msvc8-cross: Wine launcher is not executable: %s\n" "$wine" >&2
        exit 1
    fi
fi

is_crossover=
case "$wine" in
*CrossOver.app/*|*/CrossOver/*)
    is_crossover=1
    ;;
esac

mode=run
if test "${1-}" = --winepath; then
    mode=winepath
    shift
elif test "${1-}" = --; then
    shift
fi

if test "$is_crossover"; then
    bottle=${MSVC8_WINE_BOTTLE-${CX_BOTTLE-}}
    if test -n "$bottle"; then
        if test "$mode" = winepath; then
            exec "$wine" --bottle "$bottle" --no-gui --no-convert winepath -w "$@"
        fi
        exec "$wine" --bottle "$bottle" --no-gui --no-convert "$@"
    fi
    if test "$mode" = winepath; then
        exec "$wine" --no-gui --no-convert winepath -w "$@"
    fi
    exec "$wine" --no-gui --no-convert "$@"
fi

if test "$mode" = winepath; then
    exec "$wine" winepath -w "$@"
fi
exec "$wine" "$@"
