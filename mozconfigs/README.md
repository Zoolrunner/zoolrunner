# Mozconfig Examples

Mozconfig examples are organized by operating system and architecture, matching
the layout used by related UXP-based trees:

```text
mozconfigs/
  cross/
    win32-msvc8-suite-legacy.mozconfig
    win32-msvc8-suite.mozconfig
    win32-msvc8-xulrunner.mozconfig
  macos/
    arm64/
      cocoa_suite_clang.mozconfig
      cocoa_xulrunner_clang.mozconfig
  linux/
    loongarch64/
      gtk2_browser_gcc.mozconfig
      gtk2_suite_gcc.mozconfig
      gtk2_xulrunner_gcc.mozconfig
      xlib_browser_gcc.mozconfig
      xlib_suite_gcc.mozconfig
```

The cross configurations build the complete Suite or XULRunner for Windows
x86 from a Linux or macOS host with genuine Microsoft Visual C++ 2005
(MSVC 8.0 / VC8) through Wine. CrossOver is a Wine provider on macOS.
Windows 95 and Windows NT 4.0 are the minimum target operating systems;
use the legacy aggregate Suite configuration for that compatibility work.
See [the Windows build guide](../build/win32/msvc8-cross/README.md) for toolchain
layout, runtime selection, validation, static-CRT policy, and PE auditing, and
[compatibility status](../build/win32/msvc8-cross/COMPATIBILITY.md) for the
remaining runtime blockers.

The macOS arm64 Cocoa Suite and XULRunner configurations are active platform
bring-up targets. They use the macOS 11.3 SDK at
`~/dev/macos-sdk/MacOSX11.3.sdk` and are not yet verified release
configurations. Only currently exercised Linux frontends are otherwise
represented here. Do not add Qt 3
or other historical frontend mozconfigs until those frontends are known to build
and run in this tree.

Use a config by copying it to the source root as `mozconfig`, then run
`client.mk`:

```sh
cp mozconfigs/linux/loongarch64/gtk2_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

For the native Apple Silicon Cocoa Suite bring-up:

```sh
cp mozconfigs/macos/arm64/cocoa_suite_clang.mozconfig mozconfig
make -f client.mk build
```

For the native Apple Silicon Cocoa XULRunner bring-up:

```sh
cp mozconfigs/macos/arm64/cocoa_xulrunner_clang.mozconfig mozconfig
make -f client.mk build
```

For the Xlib browser build:

```sh
cp mozconfigs/linux/loongarch64/xlib_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

For the Xlib suite build:

```sh
cp mozconfigs/linux/loongarch64/xlib_suite_gcc.mozconfig mozconfig
make -f client.mk build
```
