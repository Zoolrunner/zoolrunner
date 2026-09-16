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
desktop before launching. Its diagnostic log is `/tmp/zool-startup.log`.
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

## Observed interactive limitation

Suite and Browser display their windows and local HTML in UTM, and mouse input
works. Their native menu bar is blank in the copied installer environment;
automated Command-Q attempts did not quit Browser. Use UTM Stop if necessary;
the startup script checks the persistent filesystems on the next boot. These
GUI observations do not establish complete desktop/menu/keyboard compatibility.

Browser cold-start rendering was verified. Suite rendered in the initial UTM
session, but repeated startup still opens the profile wizard even after the
profile API reports successful setup. This remains unresolved. The Calendar
and XULRunner shortcuts were not exercised in UTM; their Linux QEMU runtime/GUI
results do not substitute for that separate emulator check.
