# Calendar compatibility checks

Run the historical unit groups with `run-compatibility.py --shell PATH
--report-dir DIRECTORY`. For an uninstalled developer shell, pass its library
directory with `--library-path`. The runner selects the platform library search
variable and, on non-Darwin Unix, sets `MOZILLA_FIVE_HOME` to that directory so
classic XPCOM can also find its components. Existing library search entries
are retained after the supplied directory.

Test startup and all four views from a native Linux build:

```sh
sh build/linux/with-display.sh python3 calendar/test/run-window-compatibility.py \
  --runtime obj-speedometer21-calendar-gtk2/dist/bin \
  --report artifacts/speedometer21/calendar-views.log
```

The window runner copies the runtime, creates a private HOME/profile, and adds
`compatibility-overlay.xul` only to that temporary copy. It opens the normal
Calendar window and checks day, week, multiweek and month views. A crash,
timeout, nonzero exit or missing success marker fails the run. Existing
applications, profiles and chrome packages are not edited.

The existing macOS package mode remains available through `--archive FILE`
instead of `--runtime`. Native Linux uses the historical `sunbird-bin` program
name. The native LoongArch GTK2 and Xlib checks pass with the XHR state correction
`0d546a61`; this change does not claim new macOS runtime validation. These checks
cover startup and view switching, not every calendar provider or editing action.

The native LoongArch GTK2 package at 3c8c0eac passes all eight unit groups through
this direct invocation, plus startup and all four views. The corresponding
Xlib follow-up remains pending. Initial direct-run loader/component-path
failures are retained under `artifacts/speedometer21/` separately from the
successful `text-shadow-fractional-calendar-gtk2-units` results.
