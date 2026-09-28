# Legacy Windows Suite regression payload

The Linux/Wine CI runner also checks Browser, Calendar and XULRunner packages.
XULRunner launches the GUI fixture through `toolkit.defaultChromeURI` in its
disposable profile: its default command-line handler does not implement
Browser's `-chrome` option. The fixture still opens the unchanged Simple
application and exercises its XPT, JavaScript and native C++ components.
Subprocess output is retained even when a runtime check times out. Timeout
errors also print the application log and any fixture result to the CI log;
a stalled process remains a failure.
GUI checks wait for the private Wine session to end before reading results,
because first-run component registration can relaunch the application after
the initial process exits. Both the launch and session wait have time limits.

The host-only Calendar packaging regression needs Python, not Wine or MSVC:

```sh
python3 build/win32/msvc8-cross/tests/test-package.py
```

It stages the real component loader and its subscripts through the production
packager, removes the fixture build tree, and verifies all script bytes in both
the relocated runtime and ZIP. This catches omission of Calendar's `js/`
directory; it does not replace packaged application runtime checks.

Stage the existing platform tests from a macOS or Linux build host:

```sh
MSVC8_ROOT=/path/to/msvc8.0 MSVC8_WINE_BOTTLE=YourBottle \
python3 build/win32/msvc8-cross/tests/build-native-tests.py \
  --objdir obj-zoolrunner-win32-msvc8-suite-legacy \
  --output /path/to/new/native-tests
python3 build/win32/msvc8-cross/tests/prepare-suite-regression.py \
  --runtime /path/to/unpacked/zoolrunner \
  --xpcshell obj-zoolrunner-win32-msvc8-suite-legacy/dist/bin/xpcshell.exe \
  --native-tests /path/to/new/native-tests \
  --output /path/to/new/staging-directory
```

Use the matching MSVC2005/Wine aggregate Suite build and audit the package
first. The generator copies the package; it does not change the source package.
It includes native Expat replay, regexp cancellation and JSAPI embedding tests,
packaged interface and inline-script checks, eight JavaScript regressions and existing GUI fixtures for browser
navigation/error pages, window builtins, Suite lifecycle and ChatZilla.
This is targeted platform coverage, not the complete Test262 suite.

Put the staging directory's contents on an ISO with Joliet names. In the guest,
run `D:\setup.bat D:` (replace both drive letters with the actual CD drive).
The batch file refuses to overwrite `C:\ZRREG916`. It copies the payload there,
runs the checks and opens `report.txt`. Keep the report, `version.txt`, console
output and individual logs. Observe the GUI as well as reading the results;
a stalled process or missing report is not a pass. GUI fixtures have a two-minute
watchdog when their event loop remains responsive.

The GUI fixtures skip Suite's machine-wide default-browser prompt in their
test windows; they do not change browser associations. Suite uses its legacy
Windows integration service for this prompt, not Firefox's
`browser.shell.checkDefaultBrowser` preference.

Test the packaged runtime, not just `dist/bin`. The static Windows manifest
must ship the platform's typelibs even when native components are combined in
`mozcomps.dll`. Missing `appstartup.xpt` prevents scripted shutdown; missing
`composer.xpt` breaks the editing-session interface used by Composer.
The staging tool also rejects local chrome registrations referring to missing
JARs. Include `embed-sample.jar` when its registration is present; otherwise
fresh profiles can report XML errors while enumerating chrome packages.

The driver creates a dedicated profile and restores the previous registry
selection on completion, retaining test files for inspection. Do not rerun over
old results. Preserve or move the entire test folder before another run.
On a clean Wine prefix or Windows installation, no current profile exists yet.
The driver checks the profile count before reading the current selection and
removes its temporary selection during cleanup when there was no prior profile.
Local Linux/Wine checks on 2026-09-16 passed all 17 groups with both an empty
profile registry and a seeded existing profile; the latter retained its original
selection. These fixture rechecks used an existing Suite package and do not
constitute a fresh build or a complete `act` job.
Record the exact OS/service pack and package hashes. Results on NT4, Me or 2000
do not establish Windows 95 compatibility. The minimum targets remain Windows
95 and NT4; builds use MSVC2005 through Wine on Linux or macOS.
