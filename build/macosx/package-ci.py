#!/usr/bin/env python3
"""Package built Cocoa apps without links back into the developer checkout."""
import argparse
import os
import platform
import plistlib
import re
from pathlib import Path
import shutil
import subprocess
import tarfile
from legacy_tar import LegacyTarInfo
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("arch", choices=("arm64", "x86_64", "i386", "powerpc"))
parser.add_argument("app", choices=("suite", "browser", "calendar", "xulrunner"))
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
obj = Path(os.environ.get("ZR_OBJDIR", str(root / (
    "obj-zoolrunner-macos-%s-%s" % (args.arch, args.app)))))
dist = obj / "dist"
out = root / "artifacts"
out.mkdir(exist_ok=True)
configuration = (obj / "config/autoconf.mk").read_text()
minimum = re.search(r"^MACOSX_DEPLOYMENT_TARGET\s*=\s*(\S+)", configuration,
                    re.MULTILINE).group(1)
sdk_version = "11.3"
if args.arch == "i386":
    sdk_version = {"10.4": "10.4u", "10.8": "10.6"}[minimum]
elif args.arch == "powerpc":
    sdk_version = {"10.3.9": "10.3.9", "10.0": "10.1.5"}[minimum]
    os.environ["ZR_PPC_DEPLOYMENT_TARGET"] = minimum
legacy = args.arch in ("i386", "powerpc")
name = "zoolrunner-macos-%s-%s-sdk%s" % (args.arch, args.app, sdk_version)
if args.arch == "powerpc" and minimum == "10.0":
    # The original-library overlay has later headers; do not label it SDK 10.0.
    name = "zoolrunner-macos-powerpc-%s-target10.0" % args.app

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
    if args.arch == "powerpc" and minimum == "10.0" and args.app != "suite":
        with executable.open("rb") as binary:
            header = binary.read(28)
        if (len(header) != 28 or header[:4] != b"\xfe\xed\xfa\xce" or
                not (int.from_bytes(header[24:28], "big") & 0x8)):
            raise RuntimeError("Original-10.0 Toolkit launcher requires MH_BINDATLOAD")
        print("Original-10.0 Toolkit executable binding flag verified")
    # Match the legacy packager: generated registration caches belong to the
    # build tree and can contain stale locations/factories after relocation.
    runtime = executable.parent
    if legacy:
        # nsinstall is a build-host utility, never a runtime dependency.
        (runtime / "nsinstall").unlink(missing_ok=True)
        if args.app != "xulrunner":
            info_path = runtime.parent / "Info.plist"
            with info_path.open("rb") as info:
                metadata = plistlib.load(info)
            metadata["LSMinimumSystemVersion"] = minimum
            with info_path.open("wb") as info:
                plistlib.dump(metadata, info)
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
    if args.arch != "powerpc":
        subprocess.run(["lipo", str(executable), "-verify_arch", args.arch], check=True)
    # Compile target ABI assertions without trying to execute cross-built code.
    sdk = Path(os.environ.get("ZR_MACOS_SDK",
                              str(Path.home() / ("dev/macos-sdk/MacOSX%s.sdk" % sdk_version))))
    if args.arch == "powerpc":
        abi_compiler = [str(root / "mozconfigs/macos/powerpc/gcc")]
        if minimum == "10.0":
            shutil.copy2(sdk / "PROVENANCE.json", stage / "sdk-provenance.json")
            if not (runtime / "libzoolcxx.dylib").is_file():
                raise RuntimeError("The early C++ runtime was not installed")
            if not (runtime / "COPYING.GCC").is_file():
                raise RuntimeError("The early C++ runtime license was not installed")
    else:
        abi_compiler = ["xcrun", "clang", "-arch", args.arch, "-isysroot", str(sdk)]
    subprocess.run(abi_compiler + [
        "-I" + str(dist / "include/nspr"),
        "-I" + str(dist.parent / "js/src"), "-c",
        str(root / "build/macosx/check-target-abi.c"),
        "-o", str(Path(temporary) / "target-abi.o")], check=True)
    print("Target ABI assertions passed for " + args.arch)
    if legacy:
        audit_options = []
        if args.arch == "powerpc":
            audit_options = ["--arch", "ppc", "--tool-prefix",
                             "/opt/mac/bin/powerpc-apple-darwin8-"]
        subprocess.run([
            "python3", str(root / "build/macosx/check-legacy-binaries.py"),
            str(runtime), "--minimum", minimum, "--sdk", str(sdk),
            "--root", str(stage),
            "--report", str(stage / "deployment-checks.json")] + audit_options,
            check=True)
    # Copies need fresh ad-hoc signatures on Apple Silicon. Sign individual
    # Mach-O files after dereferencing source-tree links. No release credentials.
    for directory, _, files in os.walk(stage):
        for filename in files:
            path = Path(directory) / filename
            with path.open("rb") as binary:
                magic = binary.read(4)
            if args.arch == "i386" and magic in (
                    b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe",
                    b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"):
                subprocess.run(["lipo", str(path), "-verify_arch", "i386"],
                               check=True)
            if not legacy and magic in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe",
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
    if legacy:
        tests = stage / "runtime-tests"
        tests.mkdir()
        shutil.copy2(root / "build/macosx/verify-legacy-runtime.sh", tests / "run.sh")
        for test in ("object-reflection", "legacy-application", "debugger-lifecycle"):
            shutil.copy2(root / "js/tests/es5" / (test + ".js"), tests / (test + ".js"))
    # Original Mac OS X tar predates POSIX.1-2001 extended (PAX) headers.
    archive_format = tarfile.USTAR_FORMAT if legacy else tarfile.PAX_FORMAT
    with tarfile.open(out / (name + ".tar.gz"), "w:gz", format=archive_format,
                      tarinfo=LegacyTarInfo if legacy else tarfile.TarInfo) as archive:
        archive.add(stage, arcname=name)
    print(out / (name + ".tar.gz"))
