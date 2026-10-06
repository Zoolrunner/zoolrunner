# Building on macOS

The arm64 and x86_64 builds use **Clang and macOS SDK 11.3**. Install Xcode's
command line tools; CI uses Xcode 16.4. Use SDK 11.3 rather than the newer SDK
bundled with Xcode.

For older architectures, use the [i386](i386/README.md) or
[PowerPC](powerpc/README.md) build guide. Run commands from the repository root.

## Prerequisites

Install the host dependencies (Homebrew example):

```sh
brew install autoconf glib pkgconf perl
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk"
```

The SDK download is checksum-verified. Bundled libIDL is built automatically
using host GLib.

## Configure and build

```sh
export ZR_MACOS_SDK="$HOME/dev/macos-sdk/MacOSX11.3.sdk"
export MOZCONFIG="$PWD/mozconfigs/macos/arm64/cocoa_suite_clang.mozconfig"
make -f client.mk build
```

Choose `arm64` or `x86_64` and one of these applications:

| Profile application | Output |
| --- | --- |
| `suite` | `ZoolRunner.app` |
| `browser` | `ZoolRunner Browser.app` |
| `calendar` | `Calendar.app` |
| `xulrunner` | Runtime, Simple example and Layout Debugger |

Apple Silicon hosts can build both architectures; Intel hosts build x86_64.
Build-time tools remain native to the host. Deployment targets are 11.0 for
arm64 and 10.6 for x86_64.

The default object directory is `obj-zoolrunner-macos-ARCH-APP`. Optional
settings are `ZR_BUILD_JOBS` (default 8) and an absolute `ZR_OBJDIR`.
Run `client.mk` builds sequentially within one checkout because `.mozconfig.mk`
is shared. After changing compiler flags or SDK paths, run
`make -f client.mk configure` before rebuilding.

## Package

For the build above:

```sh
python3 build/macosx/package-ci.py arm64 suite
```

Archives appear under `artifacts/`. Packaging resolves build-tree symlinks,
checks architecture and refreshes ad-hoc signatures; these are development
packages without distribution signing or notarization. Desktop archives contain
the `.app` bundle. XULRunner archives contain `xulrunner/` and
`applications/{simple,layoutdebug}/`.

The [macOS workflow](../../.github/workflows/macos.yml) builds and packages all
four applications for both architectures.

[Runtime tests](../../build/macosx/tests/README.md),
[platform and workflow records](NOTES.md), and architecture-specific
[arm64](arm64/NOTES.md) / [x86_64](x86_64/NOTES.md) details are separate references.
