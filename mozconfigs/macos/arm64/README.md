# macOS arm64 build notes

The native arm64 configuration uses the existing Cocoa widget backend with
the Cairo/CoreGraphics graphics path. It targets macOS 11.0, the first macOS
release capable of running native Apple Silicon applications.

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
