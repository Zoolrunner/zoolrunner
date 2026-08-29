#!/bin/sh

set -e

topsrcdir=$1
objdir=$2

if test -z "$topsrcdir" -o -z "$objdir"; then
  echo "usage: bootstrap-libIDL.sh TOPSRCDIR OBJDIR" 1>&2
  exit 1
fi

source_dir="$topsrcdir/build/unix/libIDL"
workdir="$objdir/build/unix/libIDL"
srcdir="$workdir/libIDL-0.8.14"
prefix="$objdir/build/unix/libIDL-prefix"
config="$prefix/bin/libIDL-config-2"

if test -x "$config"; then
  exit 0
fi

mkdir -p "$workdir"

if test ! -d "$srcdir"; then
  mkdir -p "$srcdir"
  cp -R "$source_dir/." "$srcdir"
fi

for config_file in config.guess config.sub; do
  for config_dir in /usr/share/autoconf/build-aux /usr/share/automake-* /usr/share/libtool/build-aux /usr/share/misc; do
    if test -f "$config_dir/$config_file"; then
      cp "$config_dir/$config_file" "$srcdir/$config_file"
      break
    fi
  done
done

cd "$srcdir"

if test ! -f config.status; then
  ./configure --prefix="$prefix" --disable-shared --enable-static
fi

make
make install
