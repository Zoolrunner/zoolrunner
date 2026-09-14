#!/usr/bin/env python3
"""Test a packaged macOS Calendar in a temporary profile (requires a desktop)."""
import argparse
import os
from pathlib import Path
import plistlib
import signal
import subprocess
import tarfile
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--archive', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='zool-calendar-window-') as temporary:
    base = Path(temporary)
    with tarfile.open(args.archive) as archive:
        # Development packages contain dereferenced files, never links.
        for member in archive.getmembers():
            path = Path(member.name)
            if path.is_absolute() or '..' in path.parts or not (member.isfile() or member.isdir()):
                raise ValueError('Unsafe archive member: ' + member.name)
        archive.extractall(base)
    bundle, = base.glob('*/Calendar.app')
    with (bundle / 'Contents/Info.plist').open('rb') as info:
        program = plistlib.load(info)['CFBundleExecutable']
    runtime = bundle / 'Contents/MacOS'
    executable = runtime / program
    profile = base / 'profile'
    profile.mkdir()
    (profile / 'user.js').write_text('user_pref("browser.dom.window.dump.enabled", true);\n')
    overlay = Path(__file__).resolve().with_name('compatibility-overlay.xul')
    (base / 'compatibility-overlay.xul').write_bytes(overlay.read_bytes())
    with (runtime / 'chrome/chrome.manifest').open('a') as manifest:
        manifest.write('\ncontent calendarcompat ' + base.as_uri() + '/\n'
                       'overlay chrome://calendar/content/calendar.xul '
                       'chrome://calendarcompat/content/compatibility-overlay.xul\n')
    environment = dict(os.environ, MOZ_NO_REMOTE='1')
    environment.pop('DYLD_LIBRARY_PATH', None)
    process = subprocess.Popen([str(executable), '-profile', str(profile)],
                               env=environment, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
    timed_out = False
    try:
        output = process.communicate(timeout=45)[0]
    except subprocess.TimeoutExpired:
        timed_out = True
        # The legacy launcher may restart itself. Stop only processes using
        # this test's unique extracted executable, never another Calendar.
        for line in subprocess.check_output(['ps', '-axo', 'pid,command'], text=True).splitlines():
            fields = line.strip().split(None, 1)
            if len(fields) == 2 and fields[1].startswith(str(executable) + ' '):
                try:
                    os.kill(int(fields[0]), signal.SIGKILL)
                except ProcessLookupError:
                    pass
        output = process.communicate(timeout=5)[0]
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(output)
    print(output)
    if timed_out or process.returncode or 'CALENDAR-WINDOW views=4 failures=0' not in output:
        raise SystemExit('Calendar window compatibility failed')
