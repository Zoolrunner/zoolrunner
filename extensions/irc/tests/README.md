# Standalone XULRunner compatibility

Run the existing built ChatZilla extension as an application with its historical
application ID. The launcher copies both runtime and extension into a temporary
directory, adds a test-only startup window, and uses a private HOME/profile.
It does not edit ChatZilla scripts. No IRC servers are configured or contacted.

```sh
sh build/linux/with-display.sh python3 extensions/irc/tests/run-standalone.py \
  --runtime obj-speedometer21-xulrunner-gtk2/dist/bin \
  --extension 'obj-speedometer21-suite-merged/dist/bin/extensions/{59c81df5-4b7a-477b-912d-4e0fdf64e5f2}' \
  --report artifacts/speedometer21/xulrunner-gtk2-chatzilla.json
```

The twelve assertions cover initialization, host recognition, command and
preference managers, ES5 methods in real chrome, and the XBL input's value and
selection bindings. A timeout, crash, missing result, or failed assertion fails
the run. This is startup and local input coverage, not IRC protocol validation
or a test of every ChatZilla command. Native LoongArch GTK2 and Xlib both pass with the Speedometer engine
(`xulrunner-gtk2-chatzilla.json` and `xulrunner-xlib-chatzilla.json`).
