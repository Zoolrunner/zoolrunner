# macOS application builds

All maintained macOS profiles use **macOS SDK 11.3**, Cocoa/Cairo and Clang.
Do not substitute the SDK bundled with the host's current Xcode. The SDK's
`SDKSettings.plist` version is checked before configuration. Xcode 16.4 supplies
the compiler in CI; its bundled SDK is not used for the target build.

## Applications and configurations

Each row has both `arm64/cocoa_APP_clang.mozconfig` and
`x86_64/cocoa_APP_clang.mozconfig` under this directory:

| APP | Build output | Included applications/tools |
| --- | --- | --- |
| `suite` | `ZoolRunner.app` | Navigator, Mail & News, Composer, Address Book, ChatZilla, Venkman, DOM Inspector |
| `browser` | `ZoolRunner Browser.app` | Standalone browser |
| `calendar` | `Calendar.app` | Standalone Sunbird calendar |
| `xulrunner` | XULRunner runtime | Simple example and standalone Layout Debugger in `dist/xpi-stage` |

ChatZilla, Venkman, and DOM Inspector are Suite extensions, not independent
`--enable-application` targets. Simple and Layout Debugger use XULRunner's
mozconfig and are included in its CI archive. This does not add standalone
ChatZilla packaging.

The `mail`, `composer`, `minimo`, and `macbrowser` configure names remain
historical options, but their complete application directories are absent from
this checkout. MailNews and Composer are available within Suite. Old embedding
samples requiring other toolkits or mobile/Windows SDKs are not macOS app
profiles. `standalone` is a component-development configure mode, not a separate
complete desktop application.

## Local builds

Install Xcode's command line tools and the host dependencies (Homebrew example):

```sh
brew install autoconf glib pkgconf perl
```

libIDL 0.8.14 is built from the bundled source automatically into each object
directory; it uses host GLib. No separate upstream dependency build is required.
Use an existing SDK 11.3 installation, or fetch the checksum-pinned archive:

```sh
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk"
export ZR_MACOS_SDK="$HOME/dev/macos-sdk/MacOSX11.3.sdk"
export MOZCONFIG="$PWD/mozconfigs/macos/arm64/cocoa_calendar_clang.mozconfig"
make -f client.mk build
```

Choose `x86_64` in the path for Intel output. On an Apple Silicon host, the
existing compiler wrappers cross-compile Intel code while host tools remain
native arm64. Intel hosts can build the x86_64 profiles natively. Apple Silicon
output currently requires an Apple Silicon host. Each app/architecture gets a
separate `obj-zoolrunner-macos-ARCH-APP` directory. Run `client.mk` configurations
sequentially within one checkout because `.mozconfig.mk` is shared.

The shared `common.mozconfig` accepts `ZR_MACOS_SDK`, `ZR_BUILD_JOBS` (default 8)
and an absolute `ZR_OBJDIR` override. Run `make -f client.mk configure` explicitly
after changing compiler flags or SDK location in an existing object directory.
The existing deployment flags remain 11.0 for arm64 and 10.6 for x86_64; these
are build targets, not claims that current binaries have been tested on those
minimum OS versions. See [arm64 platform notes](arm64/README.md).

## GitHub Actions

[macos.yml](../../.github/workflows/macos.yml) builds the four applications for
both architectures on push, pull request, and manual dispatch. Its eight jobs
use GitHub's `macos-15` Apple Silicon runners, with x86_64 cross-compilation.
The workflow pins Xcode 16.4 and SDK 11.3 and verifies the SDK archive's SHA-256.
Run steps explicitly use Bash with GitHub's `-eo pipefail` options so logging
through `tee` cannot hide a failed build or packaging check.
The SDK archive comes from the
[11.3 SDK release](https://github.com/phracker/MacOSX-SDKs/releases/tag/11.3).
Runner labels and Xcode paths follow the
[GitHub runner image documentation](https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md).

Each successful job uploads a `.tar.gz` development package. Packaging resolves
build-tree symlinks, verifies the executable architecture, and refreshes ad-hoc
Mach-O signatures. Native packages also run the 101-assertion reflection smoke
test from the staged runtime, with no build-tree library-path override. Component
registration caches are removed before archiving. It does not perform
distribution signing or notarization.
XULRunner archives include `xulrunner/` and `applications/{simple,layoutdebug}/`;
launch an application with `xulrunner/xulrunner-bin applications/APP/application.ini`.
Other archives contain their `.app` bundle. Build logs and configure diagnostics
are uploaded even when compilation fails. No workflow publishes releases.

To reproduce CI packaging locally after a default-object-directory build:

```sh
python3 build/macosx/package-ci.py arm64 suite
```

Packages appear under `artifacts/`. CI execution on GitHub remains unverified
until this workflow is pushed and a hosted run completes; local validation is
recorded separately from a successful hosted run.

## Local validation (2026-09-14)

All eight profiles built successfully on macOS 15.7.1 / Xcode 16.4 with SDK
11.3, including x86_64 cross-builds on Apple Silicon. All eight development
archives were produced and their main executable architecture checked. The
four native archives pass the 101-assertion reflection smoke test after staging
outside the checkout. The workflow passes Actionlint 1.7.12; the SDK download
and checksum check pass, and configuration rejects a newer SDK.

The relocated standalone Browser opens its main window and address bar in a
fresh-profile GUI smoke test. Calendar now starts and navigates all four views
without JavaScript errors after engine fixes for historical accessor syntax and
XBL method receivers. Composer also starts and inserts text after restoring
historical regexp syntax for unversioned XUL scripts. Calendar's eight unchanged
unit tests pass, including memory and
SQLite provider operations. No Calendar application scripts were changed.
Native packaging also requires 58 legacy-language assertions and 18 JSAPI
embedding checks; Calendar packaging runs its unit tests against the staged
runtime. Intel GUI startup and minimum-OS runtime compatibility were not tested.

The new builds required small macOS fixes: Browser's bundle paths now handle
its spaced name and its plist declares the actual executable; Browser and
Calendar skip unavailable 32-bit packaging resources and link the framework
used by command-line initialization. The Browser port also replaces unavailable
64-bit Pascal-string/AppleEvent/Launch Services entry points while retaining
historical 32-bit paths where needed. Default-browser and desktop-background
changes were not exercised against the user's settings.

XULRunner packaging includes the generated runtime directories, not arbitrary
external application checkouts placed beside the developer runtime. Those local
checkouts remain untouched; Simple and Layout Debugger are packaged separately.

Native packages also pass the five debugger lifecycle checks. The desktop Suite
lifecycle regression passes 24 checks covering repeated Composer edit/undo/close
cycles and Address Book, Inspector, and Venkman startup/close, without script
console errors. See `editor/composer/tests/README.md` for reproduction.
