# Reusing the original Mac OS X 10.0 test environment in UTM

The Linux QEMU test environment can also boot in UTM's QEMU backend. Copy its
boot image and application disk into the UTM bundle; do not reference temporary
build directories. The boot image used for the recorded tests is the original
4K78 installer environment augmented with the installed system's frameworks.
This is a test environment, not a complete installed Finder desktop. The
Installer window remains behind the application.

The local copy is registered as **ZoolRunner Mac OS X 10.0**. Its launch
shortcuts, instructions and screenshot are in
`~/Documents/ZoolRunner PowerPC Tests`. All four verified target-10.0 packages
are on its application disk. The original Linux test image remains unchanged.

## Machine configuration

Use UTM's emulated PowerPC `mac99` machine, a G3 CPU, 256 MiB RAM, VGA and the
machine's ADB input devices. Set the machine property override to `via=cuda`.
Use an IDE application disk first and the copied read-only boot CD second.
The test configuration has no network adapter. Disable UEFI and USB input.

Additional QEMU arguments override UTM's default firmware startup:

```
-prom-env boot-command=boot
-prom-env boot-device=cd:2,\SystemFolderX\BootX
-prom-env boot-args=-s
```

Do not add separate `adb-kbd` or `adb-mouse` devices: the machine supplies them.
UTM 4.7.4's QEMU build rejects those explicit device names.

## Persistent test disks and startup

The copied application disk has an Apple partition map with an HFS transfer
partition (`disk0s2`), HFS+ application partition (`disk0s3`) and HFS+ root-home
partition (`disk0s4`). The latter two are formatted once when creating this
dedicated test disk. Subsequent boots check the filesystems and reuse the applications and profiles.

At the single-user `localhost#` prompt, the local VM's startup commands are:

```
/sbin/fsck_hfs -y /dev/rdisk0s2
/sbin/mount -t hfs /dev/disk0s2 /private/tmp
sh /tmp/run.sh suite &
exit
exit
```

The second `exit` acknowledges the original shell's running-job warning.
Choose `browser`, `calendar` or `xulrunner` instead of `suite` as needed. The
startup script must ignore hangup (`trap '' 1`) before entering the background:
otherwise leaving the boot console can terminate it before the GUI starts.
It checks and mounts the application and home partitions, mounts volfs, and waits for the
desktop before launching. It must then run
[`start-10.0-security.sh`](../../../build/macosx/tests/start-10.0-security.sh),
copied to the transfer partition as `/tmp/start-services.sh`, before starting
any application. The installer environment omits `lookupd` and `SecurityServer`;
the latter is required by the original OS random-number provider used by NSS.
The normal installer desktop appearing does not establish that these services
are running. Omitting this step can produce the browser's component-initialization
alert even with a writable profile. Use the same service sequence as the Linux
QEMU tests. Its diagnostic log is `/tmp/zool-startup.log`.
The interactive launcher uses [`utm-open.c`](../../../build/macosx/tests/utm-open.c)
to open the application bundle through `LSOpenFSRef`. On original 10.0, running
the Unix executable directly can display its window while leaving the native
application-name menu unresponsive. Launch Services supplies the required
application registration; this distinction is not covered by the direct-launch
GUI fixture alone. The standalone example uses a small `XULRunner Test.app`
launcher, built from [`utm-xulrunner.c`](../../../build/macosx/tests/utm-xulrunner.c),
to supply its manifest and dedicated profile. Like the existing macOS
XULRunner stub, it must retain the bundle launcher's `argv[0]`, set
`XRE_BINARY_PATH` to the actual runtime, and provide its library directory.
Using the bare runtime path as `argv[0]` loses Cocoa's bundle identity and can
leave a visible window without keyboard input. Cross-compile these helpers with
the target-10.0 toolchain; its `libzoolcxx.dylib` must be beside each helper.

The VM remains running when the application quits or relaunches. Use UTM Stop
to end a session; the next boot checks the transfer, application and home
filesystems before mounting them.

The local `.command` shortcuts automate these keystrokes through a QMP socket
in UTM's shared application-group directory. They start only a stopped VM and
leave an existing session alone. Moving the VM to another account requires
updating both its QMP argument and the helper's socket path. Manual startup
does not require QMP.

The VM images are local test assets, not repository files. For the original
Linux QEMU build/runtime results, see [10.0 status](10.0-status.md).

## Initialization and error-page checks

The initial copy omitted the required services and left an empty `key3.db`
in the Browser profile. A full profile backup was saved before moving the
failed `key3.db`, `cert8.db` and `secmod.db` aside. NSS initialization, random
bytes and shutdown subsequently passed against the persistent profile after
a cold boot. Do not delete existing users' certificate databases as a general
startup workaround.

The search-time XML error was a separate Expat pause/replay defect, corrected
in the platform parser. Thirty parser checks passed inside UTM. A toolbar
search now displays the normal server-not-found page in this offline guest;
`browser-search-fixed.png` in the local test folder records that check.
The fixture also checks DNS, offline and missing-file error-page DOMs. See
[Expat integration](../../../parser/expat/README.zoolrunner.md).

## Interactive checks

The Cocoa menu event installer previously cast an `NSView` to a Carbon
`WindowRef`, preventing menu construction on PowerPC. Application-level event
routing restores the native menu bar. Suite's File menu, Open Web Location command, application-name menu and native
Quit command were exercised in UTM. Quit closed the window and ended the Suite
process. Browser's application-name menu and native Quit were also checked. Use Launch Services as described above; a visible menu bar alone does
not establish working native commands. UTM Stop ends the remaining OS session,
and the next boot checks its writable filesystems.

Suite's initial saved `Application Registry` could not enumerate its profile
subtree. The damaged file was preserved as `Application Registry.before-repair`
in both its original directory and `/tmp/work`, then rebuilt using Suite's
`-CreateProfile` command. A separate process now finds `ZoolUTMDesktop`, and
Suite launches into the local page. Existing profile directories were retained.
The 32-bit application event loop now forwards Command shortcuts that original
AppKit does not deliver to view/window key-equivalent handlers. Native Cocoa
and Carbon menus get priority; the latter use `IsMenuKeyEvent`'s MenuRef because
system menus cannot reliably be resolved from a numeric menu ID. Browser
Command-A, copy/paste, Command-L and Command-Q have passed in the original OS.
Suite's clean-package Command-A, copy/paste and Command-Q checks also pass;
Quit closes the window and ends the process. Its Command-L address selection
and Command-H were checked with temporary tracing before the clean-package
run. Calendar's clean-package Command-A and Command-Q checks pass too. The
standalone XULRunner input check passes: Command-A replaces the textbox value
with 7, and Increment changes it to 8.
Calendar's launcher, main window, application menu and native Quit passed in UTM.
The XULRunner launcher opens the standalone Simple App, and its Increment
button was checked in UTM. This minimal example has no XUL menu bar and does
not establish XULRunner native-menu compatibility. Its window initially appears
at the bottom left; click it to activate it.
