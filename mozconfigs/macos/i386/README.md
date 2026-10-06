# Building for Mac OS X i386

Use a modern Intel or Apple Silicon Mac with Xcode 16.4's Clang and classic
i386 linker. Target code uses **SDK 10.4u**; native host tools use **SDK 11.3**.
Install the [macOS host dependencies](../README.md#prerequisites).

## Build and package

Run from the repository root:

```sh
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk" 10.4u
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk" 11.3
export ZR_MACOS_SDK="$HOME/dev/macos-sdk/MacOSX10.4u.sdk"
export ZR_HOST_SDK="$HOME/dev/macos-sdk/MacOSX11.3.sdk"
export MOZCONFIG="$PWD/mozconfigs/macos/i386/cocoa_suite_clang.mozconfig"
make -f client.mk build
python3 build/macosx/package-ci.py i386 suite
```

Replace `suite` with `browser`, `calendar` or `xulrunner` as needed. Output is
in `obj-zoolrunner-macos-i386-APP`; unsigned development archives are in
`artifacts/`. Set `ZR_BUILD_JOBS` to change parallelism or an absolute
`ZR_OBJDIR` to change the object directory.

The deployment target is 10.4. The wrappers select the SDK's startup object,
`libgcc_s.10.4` and `libstdc++.6`; do not substitute current Clang runtime
libraries. Run `client.mk` configurations sequentially in a checkout.

The [i386 workflow](../../../.github/workflows/macos-i386.yml) builds and
packages all four applications. Modern hosts cannot execute these i386
applications; [target runtime checks and recorded results](NOTES.md) are separate.
For PowerPC, use the [PowerPC build guide](../powerpc/README.md).
