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

## Observed interactive limitation

Suite and Browser display their windows and local HTML in UTM, and mouse input
works. Their native menu bar is blank in the copied installer environment;
automated Command-Q attempts did not quit Browser. Use UTM Stop if necessary;
the startup script checks the persistent filesystems on the next boot. These
GUI observations do not establish complete desktop/menu/keyboard compatibility.

Suite's initial saved `Application Registry` could not enumerate its profile
subtree. The damaged file was preserved as `Application Registry.before-repair`
in both its original directory and `/tmp/work`, then rebuilt using Suite's
`-CreateProfile` command. A separate process now finds `ZoolUTMDesktop`, and
Suite launches into the local page. Existing profile directories were retained.
The Calendar and XULRunner shortcuts have not yet been exercised in UTM; their
Linux QEMU runtime/GUI results do not substitute for that emulator check.
