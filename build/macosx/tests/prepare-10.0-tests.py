#!/usr/bin/env python3
"""Add cross-built platform probes to an already audited application archive."""
import argparse
import base64
import io
import plistlib
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from legacy_tar import LegacyTarInfo


def prepare(archive_path, obj, output):
    root = Path(__file__).resolve().parents[3]
    tests = root / "build/macosx/tests"
    dist = obj.resolve() / "dist"
    configuration = (obj / "config/autoconf.mk").read_text()
    application = re.search(r"^MOZ_BUILD_APP\s*=\s*(\S+)", configuration,
                            re.MULTILINE).group(1)
    if application not in ("suite", "browser", "calendar", "xulrunner"):
        raise ValueError("Unsupported application: " + application)
    cc = str(root / "mozconfigs/macos/powerpc/gcc")
    cxx = str(root / "mozconfigs/macos/powerpc/g++")
    if output.exists():
        raise ValueError("Refusing to overwrite " + str(output))
    with tarfile.open(archive_path, "r:gz", tarinfo=LegacyTarInfo) as original:
        shells = [PurePosixPath(m.name) for m in original.getmembers()
                  if m.isfile() and m.name.endswith("/xpcshell")]
        if len(shells) != 1:
            raise ValueError("Expected one packaged xpcshell")
        runtime = shells[0].parent
        package = PurePosixPath(runtime.parts[0])
        if application == "xulrunner":
            launch = shlex.quote("/tmp/work/" + str(runtime / "xulrunner-bin")) + " " + \
                     shlex.quote("/tmp/work/" + str(package / "applications/simple/application.ini"))
        else:
            metadata = plistlib.loads(original.extractfile(
                str(runtime.parent / "Info.plist")).read())
            launch = shlex.quote("/tmp/work/" + str(runtime / metadata["CFBundleExecutable"]))
        with tempfile.TemporaryDirectory(prefix="zool-target-tests-") as scratch:
            stage = Path(scratch)

            def compile(name, source, flags=(), cpp=False):
                subprocess.run([cxx if cpp else cc, "-O2"] +
                               [str(tests / source)] + list(flags) +
                               ["-o", str(stage / name)], check=True)

            nspr_flags = ["-I" + str(dist / "include/nspr"),
                          "-I" + str(root / "nsprpub/pr/include"),
                          "-I" + str(root / "nsprpub/pr/include/private"),
                          "-L" + str(dist / "bin"), "-lplc4", "-lplds4", "-lnspr4"]
            if (dist / "bin/XUL").is_file():
                # Resolve XUL's @executable_path dependencies while linking
                # probes in a temporary directory with the classic linker.
                graphics_libraries = [str(dist / "bin/XUL"), "-lxpcom",
                                      "-Wl,-executable_path," + str(dist / "bin")]
            else:
                graphics_libraries = ["-lthebes", "-lxpcom", "-lxpcom_core"]
            graphics_flags = ["-I" + str(root / "config/macos"),
                              "-I" + str(root / "gfx/cairo/cairo/src"),
                              "-I" + str(obj / "gfx/cairo/cairo/src"),
                              "-L" + str(dist / "bin")] + graphics_libraries + [
                              "-lplc4", "-lplds4", "-lnspr4",
                              "-framework", "Carbon"]
            compile("early-runtime", "early-runtime.cc", cpp=True)
            compile("early-ctype", "early-ctype.cpp", ["-fshort-wchar"], cpp=True)
            compile("early-ctype-stdlib", "early-ctype.cpp",
                    ["-fshort-wchar", "-DZR_STDLIB_FIRST"], cpp=True)
            compile("early-late-cocoa", "early-late-cocoa.c")
            compile("early-test-driver", "early-test-driver.c")
            compile("early-plugin.dylib", "early-nspr-plugin.c",
                    ["-dynamiclib", "-Wl,-install_name,@executable_path/early-plugin.dylib"])
            compile("early-absent.dylib", "early-nspr-dependency.c",
                    ["-dynamiclib", "-Wl,-install_name,@executable_path/early-absent.dylib"])
            compile("early-missing-plugin.dylib", "early-nspr-missing-plugin.c",
                    ["-bundle", str(stage / "early-absent.dylib")])
            (stage / "early-absent.dylib").unlink()
            compile("early-nspr", "early-nspr.c", nspr_flags)
            compile("early-regexp-abort", str(root / "js/tests/es5/TestRegExpAbort.c"),
                    ["-DXP_UNIX", "-DJS_THREADSAFE", "-DMOZILLA_1_8_BRANCH",
                     "-I" + str(dist / "include/js"),
                     "-I" + str(dist / "include/nspr"),
                     "-L" + str(dist / "bin"), "-lmozjs", "-lplc4", "-lplds4", "-lnspr4"])
            compile("early-sqlite", "early-sqlite.c",
                    ["-I" + str(root / "db/sqlite3/src"),
                     "-L" + str(dist / "bin"), "-lsqlite3"])
            compile("early-nss-sqlite", "early-sqlite.c",
                    ["-DSQLITE_THREADSAFE=1", "-I" + str(root / "security/nss/lib/sqlite"),
                     str(root / "security/nss/lib/sqlite/sqlite3.c")])
            compile("early-nss", "early-nss.c",
                    ["-I" + str(dist / "public/nss"), "-I" + str(dist / "include/nspr"),
                     "-L" + str(dist / "bin"), "-lnss3", "-lnssutil3", "-lplc4",
                     "-lplds4", "-lnspr4"])
            compile("early-quartz", "quartz-early.c", graphics_flags)
            compile("early-font", "early-font.c", graphics_flags)
            compile("early-image", "early-image.cpp",
                    ["-DMOZILLA_INTERNAL_API", "-fshort-wchar", "-include",
                     str(obj / "mozilla-config.h")] +
                    ["-I" + str(dist / "include" / name)
                     for name in ("xpcom", "string", "gfx", "nspr")] +
                    ["-I" + str(dist / "include"), "-L" + str(dist / "bin")] +
                    graphics_libraries + ["-lplc4", "-lplds4", "-lnspr4"], cpp=True)
            compile("early-cocoa", "early-cocoa.mm",
                    ["-framework", "Cocoa"], cpp=True)
            compile("early-relaunch-bin", "early-relaunch.mm",
                    [str(root / "toolkit/xre/MacLaunchHelper.m"),
                     "-I" + str(root / "toolkit/xre"), "-framework", "Cocoa"], cpp=True)
            shutil.copyfile(tests / "start-10.0-security.sh", stage / "start-security.sh")
            vectors = stage / "nss-vectors"
            vectors.mkdir()
            source_vectors = root / "security/nss/cmd/bltest/tests/chacha20_poly1305"
            for name in ("key0", "iv0", "aad0", "plaintext0"):
                shutil.copyfile(source_vectors / name, vectors / name)
            (vectors / "ciphertext.bin").write_bytes(base64.b64decode(
                b"".join((source_vectors / "ciphertext0").read_bytes().split()), validate=True))
            script = """#!/bin/sh
set -eu
cd %s
sh ./start-security.sh
./early-runtime
./early-ctype
./early-ctype-stdlib
DYLD_BIND_AT_LAUNCH=1 ./early-late-cocoa
./early-nspr ./early-plugin.dylib ./early-missing-plugin.dylib
./early-regexp-abort
./early-sqlite
./early-sqlite platform-test.db
./early-nss-sqlite
./early-nss-sqlite private-test.db
./early-nss ./nss-vectors
./early-quartz
./early-font
./early-image
./early-cocoa --window
./early-relaunch-bin
"$PWD/early-relaunch-bin"
# Original zsh resolves commands before applying a temporary PATH assignment.
PATH="$PWD:$PATH"
export PATH
early-relaunch-bin
if sh /tmp/work/%s/runtime-tests/run.sh "$PWD" /tmp/work/js-results; then
    cat /tmp/work/js-results/*.log
else
    cat /tmp/work/js-results/*.log
    exit 1
fi
echo "Packaged platform runtime checks passed"
""" % (shlex.quote(str(runtime)), shlex.quote(str(package)))
            boundary_paths = ["tar-boundary/" + "a" * (length - len("tar-boundary/"))
                              for length in (99, 100, 101)]
            boundary_checks = "".join(
                'test "$(cat /tmp/work/%s)" = "USTAR boundary preserved"\n' % name
                for name in boundary_paths)
            script = script.replace("sh ./start-security.sh", boundary_checks +
                                    'echo "Original tar filename boundaries passed"\n' +
                                    "sh ./start-security.sh", 1)
            profile = "/tmp/work/graphical-profile"
            gui_arguments = "-chrome chrome://zooltest/content/early-application.xul"
            if application == "calendar":
                gui_arguments = "-zoolrunner-test"
            chrome_dir = "/tmp/work/" + str(runtime / "chrome")
            if application == "xulrunner":
                chrome_dir = "/tmp/work/" + str(package / "applications/simple/chrome")
            # The Suite profile service appends the name to an explicit path.
            if application == "suite":
                profile += "/ci"
            script += """
mkdir -p %s
echo 'content zooltest file:///tmp/work/' > %s/zooltest.manifest
echo 'content,install,url,file:///tmp/work/' >> %s/installed-chrome.txt
""" % (shlex.quote(chrome_dir), shlex.quote(chrome_dir), shlex.quote(chrome_dir))
            script += """
mkdir -p %s
cat > %s/prefs.js <<'PREFS'
user_pref("browser.shell.checkDefaultBrowser", false);
user_pref("browser.dom.window.dump.enabled", true);
user_pref("app.update.enabled", false);
user_pref("browser.startup.homepage", "about:blank");
user_pref("browser.startup.page", 0);
user_pref("toolkit.defaultChromeURI", "chrome://zooltest/content/early-application.xul");
user_pref("zoolrunner.test.application", "%s");
user_pref("zoolrunner.test.profile", "%s");
PREFS
profile_status=0
%s -CreateProfile "ci /tmp/work/graphical-profile" || profile_status=$?
# Historical profile creation terminates through NS_ERROR_ABORT. The GUI
# checks below must verify the selected profile and complete successfully.
case "$profile_status" in 0|1) ;; *) exit "$profile_status" ;; esac
%s -P ci %s
# Toolkit applications can asynchronously relaunch during first-run setup.
# The result comes from the final GUI process, not the initial launcher.
attempt=0
while test ! -f /tmp/application-result.txt && test "$attempt" -lt 120; do
    sleep 1
    attempt=$((attempt + 1))
done
cat /tmp/application-result.txt
awk 'NR==1 {if ($0 != "PASS") exit 1; found=1} END {if (!found) exit 1}' /tmp/application-result.txt
echo "Packaged application GUI checks passed"
""" % (profile, profile, application, profile, launch, launch, gui_arguments)
            # Repack USTAR directly, preserving the audited application bytes.
            with output.open("xb") as destination:
                with tarfile.open(fileobj=destination, mode="w:gz", format=tarfile.USTAR_FORMAT,
                                  tarinfo=LegacyTarInfo) as tar:
                    for member in original.getmembers():
                        tar.addfile(member, original.extractfile(member) if member.isfile() else None)
                    if application == "calendar":
                        tar.add(tests / "early-calendar-commandline.js",
                                arcname=str(runtime / "components/zool-test-commandline.js"))
                    for entry in stage.iterdir():
                        tar.add(entry, arcname=str(runtime / entry.name))
                    for name in ("early-application.xul", "early-page.html"):
                        tar.add(tests / name, arcname=name)
                    tar.add(tests / "early-contents.rdf", arcname="contents.rdf")
                    for name in boundary_paths:
                        data = b"USTAR boundary preserved\n"
                        entry = LegacyTarInfo(name)
                        entry.size = len(data)
                        entry.mode = 0o644
                        tar.addfile(entry, io.BytesIO(data))
                    entrypoint = "#!/bin/sh\nset -eu\ncd %s\nexec ./early-test-driver /tmp/work/checks.sh\n" % shlex.quote(str(runtime))
                    for name, text in (("test.sh", entrypoint), ("checks.sh", script)):
                        # checks.sh starts from the work root, independent of
                        # the driver's location beside its runtime libraries.
                        if name == "checks.sh":
                            text = text.replace("set -eu\n", "set -eu\ncd /tmp/work\n", 1)
                        data = text.encode()
                        entry = tarfile.TarInfo(name)
                        entry.mode = 0o755
                        entry.size = len(data)
                        tar.addfile(entry, io.BytesIO(data))
    print("Prepared packaged application tests: " + str(output))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("obj", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    prepare(args.archive, args.obj, args.output)
