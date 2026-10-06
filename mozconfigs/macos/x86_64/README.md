# Building for macOS x86_64

Use an Intel or Apple Silicon Mac, Clang and **SDK 11.3**. Follow the
[macOS prerequisites](../README.md#prerequisites), then run from the repository
root:

```sh
export ZR_MACOS_SDK="$HOME/dev/macos-sdk/MacOSX11.3.sdk"
export MOZCONFIG="$PWD/mozconfigs/macos/x86_64/cocoa_suite_clang.mozconfig"
make -f client.mk build
python3 build/macosx/package-ci.py x86_64 suite
```

Replace `suite` with `browser`, `calendar` or `xulrunner` as needed. Output is
in `obj-zoolrunner-macos-x86_64-APP`; archives are in `artifacts/`. The deployment
target is Mac OS X 10.6, and packages contain only x86_64 code.

On Apple Silicon, host tools remain native arm64. Rosetta is needed to execute
the Intel package checks, not to compile the target.

[Platform limitations](NOTES.md) are documented separately.
