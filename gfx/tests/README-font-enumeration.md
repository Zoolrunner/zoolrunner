# Font enumeration regression

`font-enumeration.xul` calls the native font enumerator while its chrome window
is being parsed, before initial layout. Its twelve checks cover installed names,
count/array agreement, null and empty language filters, a language-specific
subset, sorting and stability of separately returned arrays. GTK's existing
generic-family prefix is preserved rather than counted as an installed name
for the sorting assertion. The native Linux package runner runs this fixture
for all four applications.

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-toolkit.py \
  --application browser --runtime obj-speedometer21-browser-xlib/dist/bin \
  --chrome-probe gfx/tests/font-enumeration.xul \
  --report artifacts/speedometer21/browser-fonts.json
```

Native LoongArch GTK2 Browser and Xlib Browser/Suite pass. Xlib previously
crashed in `EnumFonts` while opening Browser's real preference panes: a cached
font-context pointer was set only by `FamilyExists` and was not owned or cleared
with its device. Enumeration now holds an initialized screen device for the
duration of the operation and copies the names before releasing it. An absent
application display fails safely with `NS_ERROR_NOT_AVAILABLE`. Xprint shares
the source but does not register the screen enumerator; its separate compilation
does not acquire a screen device.

Unfiltered enumeration also no longer mistakes a null language for a literal
language filter. Existing Xlib generic-family filtering/default-font limitations
remain; these tests do not establish complete font selection correctness.
Browser's preference fixture additionally requires a populated font menu and a
completed font list, since the original font builder catches enumeration errors.
Its expanded thirty checks pass on both native LoongArch backends.

The Xlib-only `font-enumeration-headless.js` shell test covers both enumeration
entry points before application-display initialization. It requires the
documented safe failure rather than a crash or an empty success result. Both
checks pass in native LoongArch Browser and Suite; Linux Xlib package validation
runs them before starting any application windows.
