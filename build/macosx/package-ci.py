#!/usr/bin/env python3
"""Package built Cocoa apps without links back into the developer checkout."""
import argparse
import os
import platform
import plistlib
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("arch", choices=("arm64", "x86_64"))
parser.add_argument("app", choices=("suite", "browser", "calendar", "xulrunner"))
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
dist = root / ("obj-zoolrunner-macos-%s-%s" % (args.arch, args.app)) / "dist"
out = root / "artifacts"
out.mkdir(exist_ok=True)
name = "zoolrunner-macos-%s-%s-sdk11.3" % (args.arch, args.app)

with tempfile.TemporaryDirectory(prefix="zool-package-") as temporary:
    stage = Path(temporary) / name
    stage.mkdir()
    if args.app == "xulrunner":
        runtime_directories = {"chrome", "components", "defaults", "extensions",
                               "greprefs", "plugins", "res"}
        def ignore_developer_files(directory, names):
            ignored = {name for name in names if name in (".git", ".svn", ".DS_Store")}
            if Path(directory) == dist / "bin":
                # Developers may keep external application checkouts beside
                # the runtime. Package only the runtime's generated directories.
                ignored.update(name for name in names
                               if (Path(directory) / name).is_dir()
                               and name not in runtime_directories)
            return ignored
        shutil.copytree(dist / "bin", stage / "xulrunner", symlinks=False,
                        ignore=ignore_developer_files)
        for app in ("simple", "layoutdebug"):
            shutil.copytree(dist / "xpi-stage" / app, stage / "applications" / app,
                            symlinks=False)
        executable = stage / "xulrunner" / "xulrunner-bin"
    else:
        bundle = {"suite": "ZoolRunner.app", "browser": "ZoolRunner Browser.app",
                  "calendar": "Calendar.app"}[args.app]
        shutil.copytree(dist / bundle, stage / bundle, symlinks=False)
        with (stage / bundle / "Contents" / "Info.plist").open("rb") as info:
            program = plistlib.load(info)["CFBundleExecutable"]
        executable = stage / bundle / "Contents" / "MacOS" / program
    # Match the legacy packager: generated registration caches belong to the
    # build tree and can contain stale locations/factories after relocation.
    runtime = executable.parent
    def remove_registration_caches():
        for relative in ("components/compreg.dat", "components/xpti.dat",
                         "chrome/chrome.rdf", "chrome/app-chrome.manifest",
                         "chrome/overlayinfo"):
            cached = runtime / relative
            if cached.is_dir():
                shutil.rmtree(cached)
            elif cached.exists():
                cached.unlink()
    remove_registration_caches()
    # Check the artifact itself, not a potentially different dist/bin executable.
    subprocess.run(["lipo", str(executable), "-verify_arch", args.arch], check=True)
    # Copies need fresh ad-hoc signatures on Apple Silicon. Sign individual
    # Mach-O files after dereferencing source-tree links. No release credentials.
    for directory, _, files in os.walk(stage):
        for filename in files:
            path = Path(directory) / filename
            with path.open("rb") as binary:
                magic = binary.read(4)
            if magic in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe",
                         b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"):
                # Classic Mozilla keeps resources under Contents/MacOS. Modern
                # codesign misclassifies those as nested code when signing the
                # launcher in place, so sign each executable outside the bundle.
                signing = Path(temporary) / "signing"
                signing.mkdir(exist_ok=True)
                signed = signing / path.name
                shutil.copy2(path, signed)
                subprocess.run(["codesign", "--force", "--sign", "-", str(signed)],
                               check=True)
                os.replace(signed, path)
    if platform.machine() == args.arch:
        environment = dict(os.environ)
        environment.pop("DYLD_LIBRARY_PATH", None)
        for test, marker in (
                ("object-reflection.js", "ES5-OBJECT-REFLECTION checks=101 failures=0"),
                ("legacy-application.js", "LEGACY-APPLICATION checks=58 failures=0"),
                ("debugger-lifecycle.js", "DEBUGGER-LIFECYCLE checks=5 failures=0")):
            result = subprocess.run(
                [str(runtime / "xpcshell"), "-f", str(root / "js/tests/es5" / test)],
                env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, timeout=60)
            print(result.stdout)
            if result.returncode or marker not in result.stdout:
                raise RuntimeError("Packaged runtime failed " + test)
        # A shell alone cannot exercise embedding object scope chains or
        # security callbacks during lazy Object/Function initialization.
        embedding = Path(temporary) / "embedding-test"
        sdk = Path(os.environ.get("ZR_MACOS_SDK",
                                  str(Path.home() / "dev/macos-sdk/MacOSX11.3.sdk")))
        with (sdk / "SDKSettings.plist").open("rb") as info:
            if plistlib.load(info).get("Version") != "11.3":
                raise RuntimeError("Embedding checks require macOS SDK 11.3")
        subprocess.run([
            "xcrun", "clang", "-isysroot", str(sdk), "-DXP_UNIX", "-DJS_THREADSAFE", "-DMOZILLA_1_8_BRANCH",
            "-I" + str(dist / "include/js"), "-I" + str(dist / "include/nspr"),
            str(root / "js/tests/es5/TestObjectEmbedding.c"),
            "-L" + str(runtime), "-lmozjs", "-o", str(embedding)], check=True)
        embedding_env = dict(environment, DYLD_LIBRARY_PATH=str(runtime))
        result = subprocess.run([str(embedding)], env=embedding_env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=60)
        print(result.stdout)
        if result.returncode or "failures=0" not in result.stdout:
            raise RuntimeError("Packaged runtime failed embedding compatibility")
        if args.app == "calendar":
            subprocess.run([
                "python3", str(root / "calendar/test/run-compatibility.py"),
                "--shell", str(runtime / "xpcshell"),
                "--report-dir", str(out / (name + "-calendar-tests"))],
                env=environment, check=True)
        remove_registration_caches()
    shutil.copy2(root / "LICENSE", stage / "LICENSE")
    with tarfile.open(out / (name + ".tar.gz"), "w:gz") as archive:
        archive.add(stage, arcname=name)
    print(out / (name + ".tar.gz"))
