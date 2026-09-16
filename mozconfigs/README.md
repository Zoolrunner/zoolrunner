# Application mozconfigs

Every OS and architecture represented here has profiles for **Suite, Browser,
Calendar and XULRunner**. Select a profile with an absolute `MOZCONFIG` path;
shared profiles use that path to locate the checkout.

| Target | Architectures | Profiles |
| --- | --- | --- |
| Linux / GTK2 and Xlib | i686, x86_64 | `<arch>/{gtk2,xlib}_<app>_gcc.mozconfig` under `linux/` |
| Linux / GTK2 and experimental Xlib | loongarch64 | `linux/loongarch64/{gtk2,xlib}_<app>_gcc.mozconfig` |
| macOS / Cocoa | arm64, x86_64, i386, powerpc | `macos/<arch>/cocoa_<app>_{clang,gcc}.mozconfig` |
| Windows x86 | i586 | `cross/win32-msvc8-<app>.mozconfig`; use `suite-legacy` for Suite |

Linux i686 and x86_64 builds use **Oracle Linux 8 containers and GCC Toolset
14**, both locally and in CI. See [the Linux build guide](../build/linux/README.md).
Build and runtime verification is in progress. Newly added LoongArch Calendar,
Xlib Calendar and Xlib XULRunner profiles are unverified; existing profiles
are retained independently of the x86 bring-up.

Modern macOS arm64 and x86_64 profiles require SDK **11.3**. The i386 and
PowerPC profiles use their respective legacy SDKs and toolchains, documented
in [the macOS guide](macos/README.md).

Windows builds use genuine **MSVC 2005 through Wine on Linux or macOS**.
The minimum targets remain **Windows 95 and NT 4.0**. See [the Windows build
guide](../build/win32/msvc8-cross/README.md) for validation status.

Example, from the appropriate Linux container:

```sh
export MOZCONFIG="$PWD/mozconfigs/linux/x86_64/gtk2_suite_gcc.mozconfig"
make -f client.mk build
```

Do not infer tested support solely from the presence of a profile. Preserve
LoongArch and existing toolkit profiles when fixing other architectures.
