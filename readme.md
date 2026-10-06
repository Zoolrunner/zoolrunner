# ZoolRunner

ZoolRunner is an updated and modernized platform for developing and running
**XULRunner 1.8.1 / Mozilla 1.8.1 applications**. It is a fork of RetroZilla,
which was derived from Mozilla 1.8.1.

The goal is to keep applications and code from that era working while improving
the implementation underneath: fixing bugs, updating dependencies and adding
modern language, rendering and operating system support. XUL, XBL, XPCOM,
XPConnect and the classic Mozilla application interfaces remain central to it.

The primary goal is the **application platform**. The browser and full Mozilla
Suite are included in the source tree and maintained as usable applications,
as well as tests of the platform. You can use ZoolRunner as a browser or Suite,
or use its runtime to build and run standalone applications.

## Platforms

ZoolRunner targets:

- **Mac OS X / macOS:** All releases from 10.0 to the latest release, across PowerPC, Intel
  and Apple Silicon.
- **Linux:** ARM64, x86, x86_64 and LoongArch64.
- **Windows:** Windows 95 and Windows NT 4.0 onward.

See the [documentation](docs/README.md) for build instructions and the tested
versions and configurations within these compatibility goals.

Mozilla and RetroZilla authorship and attribution are preserved. See
[LICENSE](LICENSE), [LEGAL](LEGAL) and individual source files for license terms
and credits.
