# Application lifecycle regression

From a macOS desktop session with the Suite built, run:

```sh
python3 editor/composer/tests/run-lifecycle.py \
  --runtime obj-zoolrunner-macos-arm64-suite/dist/bin \
  --report /tmp/zool-platform-lifecycle.log
```

Require `PLATFORM-LIFECYCLE checks=24 failures=0`. The runner creates and removes
its own Suite profile, registers the fixture only in that profile, and restores
the previously selected profile. It also accepts a packaged Suite's
`ZoolRunner.app/Contents/MacOS` directory. Do not run it concurrently with other
tests that change Suite's selected profile.

The fixture performs three Composer startup/edit/undo/close cycles. It verifies
that command observers work during editing and stop after close, even with an
editor reference retained and an observer deliberately left registered. The
wait after close exceeds the selection update timer delay. It then opens and
closes Address Book, DOM Inspector, and Venkman and rejects script console
errors. These are unchanged application scripts.

The platform updater must become inactive when its document is destroyed or
its docshell starts destruction. Canceling a timer alone is insufficient:
selection callbacks can rearm it, and retained editors can outlive their window
script globals. Each command in a notification group must recheck lifecycle
state because an observer can synchronously close the window.

`js/tests/es5/debugger-lifecycle.js` separately exercises script enumeration
while callbacks trigger GC, turn debugging off, and restart it. JSD snapshots
reference-counted wrappers before invoking callers; a raw iterator cannot safely
survive those callbacks. Native macOS packaging runs this regression for all
four application runtimes. The GUI lifecycle check requires a desktop session
and is separate from hosted CI.
