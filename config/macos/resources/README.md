# Classic Mac resources for Linux cross-builds

These architecture-independent resource files preserve the resources generated
by the existing Mac build. Linux has no Apple Rez or SDP tools. The cross-build
copies these files only after checking both the original source and generated
resource against `manifest.json`; source changes therefore fail closed until
the resources are regenerated. Native Mac builds still run Apple's tools.
The original source files retain their historical licensing and attribution.

Regenerate from the repository root on macOS with Xcode's command-line tools:

```sh
xcrun Rez -i "$MACOS_SDK_DIR/Developer/Headers/FlatCarbon" -useDF \
  widget/src/mac/nsMacWidget.r -o config/macos/resources/libwidget.rsrc
xcrun sdp -fa -o /tmp/zoolrunner-mozillaSuite.r \
  xpfe/bootstrap/appleevents/mozilla.sdef
xcrun Rez -useDF /tmp/zoolrunner-mozillaSuite.r \
  -o config/macos/resources/mozillaSuite.rsrc
```

Use SDK 10.4u for `MACOS_SDK_DIR`. These copies were generated with Xcode 16.4.
Update the manifest's SHA-256 values for the two source/resource pairs after
reviewing the regenerated files. This preserves the existing dictionary
generation behavior; it does not establish AppleScript runtime compatibility.
