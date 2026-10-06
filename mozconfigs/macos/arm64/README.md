# Building for macOS arm64

Use an Apple Silicon Mac, Clang and **SDK 11.3**. Follow the
[macOS prerequisites](../README.md#prerequisites), then run from the repository
root:

```sh
export ZR_MACOS_SDK="$HOME/dev/macos-sdk/MacOSX11.3.sdk"
export MOZCONFIG="$PWD/mozconfigs/macos/arm64/cocoa_suite_clang.mozconfig"
make -f client.mk build
python3 build/macosx/package-ci.py arm64 suite
```

Replace `suite` with `browser`, `calendar` or `xulrunner` as needed. Output is
in `obj-zoolrunner-macos-arm64-APP`; archives are in `artifacts/`. The deployment
target is macOS 11.0, and packages contain only arm64 code.

If a rebuilt development XULRunner is killed with `Taskgated Invalid Signature`,
refresh its ad-hoc signature:

```sh
codesign --force --sign - obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xulrunner-bin
```

[Platform limitations and keyboard regression checks](NOTES.md) are documented
separately.
