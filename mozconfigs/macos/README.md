# macOS application builds

The arm64 and x86_64 macOS profiles use **macOS SDK 11.3**, Cocoa/Cairo and Clang.
Do not substitute the SDK bundled with the host's current Xcode. The SDK's
`SDKSettings.plist` version is checked before configuration. Xcode 16.4 supplies
the compiler in CI; its bundled SDK is not used for the target build.

Experimental [i386 cross-build instructions and PowerPC research](i386/README.md)
are separate: i386 target code uses SDK 10.4u with a 10.4 deployment target,
while build-host utilities still use SDK 11.3. Do not infer runtime support
from a successful cross-build.

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
runtime. Native embedding checks have a 180-second per-test limit, overridable
with `ZR_MACOS_EMBEDDING_TIMEOUT`; the package runner prints each test name and
its captured output if one times out. Intel GUI startup and minimum-OS runtime
compatibility were not tested.

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

## Local GitHub Actions testing with act

On a Mac with the build dependencies installed, run the workflow with act's
native host runner and a local artifact server:

```sh
act push -W .github/workflows/macos.yml \
  -P macos-15=-self-hosted --concurrent-jobs 1 \
  --matrix arch:arm64 --matrix app:suite \
  --env "DEVELOPER_DIR=$(xcode-select -p)" \
  --artifact-server-addr 127.0.0.1 \
  --artifact-server-path /tmp/zool-act-artifacts
```

Omit the two `--matrix` filters to exercise all eight jobs. The native runner
uses the host's tools; it does not emulate GitHub's macOS image. Checkout,
dependency setup, SDK download/checksum, configure/build, package checks, and
artifact uploads execute inside act. No GitHub token is needed for the local
artifact server. Use a different `--artifact-server-port` for simultaneous act
invocations. The workflow keeps installed Homebrew packages and makes any
required installation noninteractive, without upgrading unrelated dependents.

A local act pass is separate from a successful run on GitHub-hosted runners.

### act artifact-server limitation

act 0.2.87 rejects the `mime_type` field sent by the pinned
`actions/upload-artifact@v7.0.1`, so both upload steps fail against its local
artifact server even when build and packaging succeed. This is an upstream act
compatibility issue, also reported for 0.2.89:
<https://github.com/nektos/act/issues/6114>. Keep the production action pinned;
do not interpret these local upload failures as successful end-to-end jobs.
Generated archives and package logs remain in the failed job's host workspace
under act's cache. GitHub-hosted artifact uploads still require verification.

### Recorded act validation (2026-09-14)

The actual workflow ran under act 0.2.87's native host runner on macOS 15.7.1
with Xcode 16.4 and SDK 11.3, using isolated checkouts for each matrix entry.
The local Xcode path was supplied through `DEVELOPER_DIR` as shown above.

| Application | arm64 build/package | x86_64 build/package |
| --- | --- | --- |
| Suite | Passed | Passed |
| Browser | Passed | Passed |
| Calendar | Passed | Passed |
| XULRunner | Passed | Passed |

Each arm64 package passed 101 reflection, 58 legacy-language, five debugger
lifecycle, and 18 embedding assertions. Calendar also passed all eight existing
unit tests. Intel packages passed architecture and packaging checks; runtime
checks were limited to the native arm64 packages. All eight jobs failed their
artifact uploads with the act limitation above, so this is **not** a passing
end-to-end CI run. The workflow also passes Actionlint 1.7.12. Testing found and
fixed a Homebrew confirmation prompt that prevented unattended dependency setup.

### Modern matrix rerun (2026-09-15)

All eight jobs now pass end to end, including both artifact uploads, using act
0.2.89 with the upstream PR 6115 artifact-server fix applied locally. This
reruns the actual pinned workflow with SDK 11.3 and Xcode 16.4 on macOS 15.7.1.
Jobs ran sequentially, and completed temporary checkouts, objects and SDK
copies were removed after preserving their artifacts and logs. Uploaded ZIP
integrity and the contained archive checksums were verified.

Packaging now executes the Intel runtime checks through Rosetta on Apple
Silicon and compiles the embedding probe for the package architecture. Both
architectures pass the reflection, historical-language, debugger lifecycle and
18 embedding assertions; both Calendar packages pass all eight unit tests.
This supersedes the runtime and local-upload limitations of the September 14
run above. It does not establish a GitHub-hosted pass or execution on physical
Intel hardware or the minimum deployment OS.

The relocated-package desktop regressions and their reproduction commands are
documented in [macOS runtime tests](../../build/macosx/tests/README.md).
All eight packages pass those GUI, focused JavaScript, regexp cancellation,
image-buffer and relaunch checks. Suite passes 24 Composer/Address Book/
Inspector/Venkman lifecycle checks and initializes ChatZilla on each architecture.
Both Calendar builds open all four views, and both XULRunner builds pass the
169 HTML/CSS layout assertions. Native and emulated early-Quartz stroke/pixel
paths and all nine opacity cases pass for each architecture. These automated
checks do not replace the manual keyboard-layout and IME checklist.

Every relocated package also passes all **11,540 ES5 Test262 cases** at revision
`7da91bceb9ce7613f87db47ddd1292a2dda58b42`, using the upstream default execution
policy. Across all eight packages there are no failed cases, timeouts, crashes
or harness errors. Fresh Intel runtimes were started once before the suite to
allow initial Rosetta startup; actual cases retained the ten-second timeout.
This is a result for the pinned suite, not proof of complete specification
conformance. Local reports, logs and archive hashes are preserved under
`artifacts/macos-modern-validation/`, with the summary in `results.json`.

Package validation initializes the freshly copied runtime in a separate process
before running JavaScript fixtures. Cold Suite component registration was
measured at 100.6 seconds under Rosetta during concurrent validation, before
JavaScript execution. Initialization has a 180-second limit and must print its
readiness marker; every fixture keeps its existing timeout and assertions.
The original 60-second first-fixture failures and loader samples remain under
`artifacts/es6/discarded-suite-*` and `discarded-effects-x86-*`.
