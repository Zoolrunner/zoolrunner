# macOS arm64 build notes

The native arm64 configuration uses the existing Cocoa widget backend with
the Cairo/CoreGraphics graphics path. It targets macOS 11.0, the first macOS
release capable of running native Apple Silicon applications.

The package qualifier is `arm64`, producing a DMG named like
`zoolrunner-2.3.en-US.mac-arm64.dmg`. This package is not a Universal Binary.

The NPAPI host and its application-facing interfaces remain built. The
bundled Default Plugin sample is not built for arm64: its implementation and
Xcode project depend on the 32-bit QuickDraw plug-in drawing/event model,
which is absent from the 64-bit macOS ABI. Historical PowerPC and Intel Mac
configurations continue to select the original sample build.

Third-party NPAPI plug-in bundles must contain a compatible arm64 Mach-O
executable. On LP64, ZoolRunner reads their established `WebPluginMIMETypes`
Info.plist metadata rather than requiring a classic resource fork.

The historical Mac NPAPI paint and event bridge passes QuickDraw ports and
Carbon `EventRecord` structures. The Cairo Cocoa backend does not expose a
QuickDraw plug-in port, so that bridge is retained for the older graphics
backend but is not enabled in the arm64 configuration. A usable arm64 plug-in
would require a compatible non-QuickDraw drawing and event model.

The historical `PrintPDE.plugin` is also retained in source but is not built
for arm64. It implements Apple's retired Carbon Print Dialog Extension ABI,
which is not available to 64-bit applications. The arm64 build uses the Cocoa
print and page-setup panels and preserves the native PrintCore settings used by
the existing Mozilla printing interfaces.

The XPFE AppleScript object-model implementation is preserved for historical
Mac targets. Its window and document handlers require 32-bit-only QuickDraw,
FSSpec, AEPackObject, and resource-fork interfaces, so arm64 currently provides
only the existing startup entry points and lets Cocoa handle ordinary
application lifecycle events. Unsupported legacy scripting events fail without
entering another architecture's implementation.


## Startup keyboard regression

The Cocoa window backend orders an ordinary window and requests activation
through `NSApplication`. Activating only the process with `NSRunningApplication`
can leave AppKit's active/text-input state behind the visible window state.
The Suite no longer sends a separate early process activation request before
its first window exists. If a window becomes key before a Gecko view becomes
its first responder, that view supplies the missed Gecko activation event when
it accepts focus. Popups and invisible windows do not request activation.

Use a disposable profile for these checks; do not terminate an existing user
process or automate against its profile. Rebuild the app bundle as well as the
widget component before testing. Test both Finder launch and direct executable
launch, and repeat with a standalone XUL application when changing shared
widget code.

1. Quit the test application completely and start it again. Before switching
   to another application, click its address bar or a XUL textbox and type.
   Confirm that characters appear, rather than merely observing key events.
2. Press Command-A, type replacement text, and check that it replaces the
   selection. Exercise Backspace, arrows, and Command-C/Command-V as well.
3. Open `widget/src/cocoa/tests/startup-keyboard.html` in the browser and repeat
   with its input and textarea. For a cold-start content test, supply that file
   as the startup URL and click a field before touching browser chrome.
4. Repeat in a second window, then switch applications and back. Verify that
   typing still works and focus returns to the expected field.
5. With a suitable keyboard layout, also check an Option/dead-key accent and
   IME composition. Do not bypass AppKit text interpretation to fix startup.

During the 2026-09-14 investigation on macOS 15.7.1 arm64, native probes observed
an inactive application at startup with the previous process-activation path.
The updated Suite reached active/key-window state and delivered native text
callbacks without switching applications. On 2026-09-14, the user confirmed
that the rebuilt app resolves the reported issue: typing works immediately
after startup without switching to another application and back. This confirms
the reported startup regression; it does not establish completion of every
keyboard-layout, IME, or additional-window check above.

The Suite and XULRunner builds succeeded; the
Suite also passed the existing 169 layout assertions and live HTTPS navigation
probe with the activation changes. The standalone XULRunner application passed
the same 169 layout assertions.

### Local XULRunner signature after rebuilding

During the 2026-09-14 bootstrap validation, the rebuilt development XULRunner
launcher was killed before startup with `Taskgated Invalid Signature`, although
`codesign --verify` accepted it on disk. Refreshing its local ad-hoc signature
restored startup:

```sh
codesign --force --sign - obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xulrunner-bin
```

After that refresh, unchanged ChatZilla and the chrome/content window bootstrap
test both passed. This is a local development signature, not distribution
signing or notarization.
