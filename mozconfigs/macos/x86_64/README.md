# macOS x86_64 build notes

The x86_64 configuration uses the existing Cocoa widget backend with the
Cairo/CoreGraphics graphics path.  It targets macOS 10.6, the first release
whose Intel kernel and userland are fully 64-bit, while building against the
macOS 11.3 SDK in `~/dev/macos-sdk`.

This configuration is an Apple Silicon cross-build: target libraries and
applications are Mach-O x86_64, while build-time tools are native arm64.
Rosetta is useful for testing the finished Intel application, but is not used
as the target architecture or to build host tools.

The package qualifier is `x86_64`, producing a DMG named like
`zoolrunner-2.3.en-US.mac-x86_64.dmg`. This package contains only the Intel
slice and is not a Universal Binary.

The bundled Default Plugin sample is not built for x86_64.  Its implementation
and Xcode project depend on the 32-bit QuickDraw plug-in drawing/event model.
The NPAPI host and its application-facing interfaces remain built; a usable
third-party plug-in must contain a compatible x86_64 Mach-O executable.

The bundled Java/OJI plug-in executables contain only PowerPC and i386 code,
so they are retained in source but not packaged in the x86_64 application.

The historical `PrintPDE.plugin`, QuickTime widget linkage, AGL bridge, Rez
resources, and full XPFE AppleScript object model use APIs unavailable to
64-bit applications.  They remain selected for the historical 32-bit PowerPC
and Intel builds.  The x86_64 build shares the existing LP64 Cocoa printing,
widget, graphics, Dock integration, and safe AppleEvents startup paths with
the arm64 build.
