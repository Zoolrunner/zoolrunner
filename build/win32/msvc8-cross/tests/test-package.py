#!/usr/bin/env python3
"""Host-only package-content regression; does not execute Windows binaries."""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[4]


class CalendarPackage(unittest.TestCase):
    def test_component_subscripts_survive_relocation(self):
        source = ROOT / 'calendar/base/src'
        module = source / 'calItemModule.js'
        # Follow the production component loader's dependencies, rather than
        # duplicating the packager's directory allowlist in this assertion.
        names = set(re.findall(r'script:\s*"([^"]+)"', module.read_text()))
        self.assertIn('calItemBase.js', names)
        expected = {'components/calItemModule.js': module.read_bytes()}
        expected.update(('js/' + name, (source / name).read_bytes()) for name in names)
        with tempfile.TemporaryDirectory(prefix='calendar package ') as tmp:
            obj = Path(tmp) / 'obj'
            work = Path(tmp) / 'work'
            for name in ['logs', 'artifacts']:
                (work / name).mkdir(parents=True)
            for name in expected:
                dest = obj / 'dist/bin' / name
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.symlink_to(source / dest.name)
            subprocess.run([sys.executable,
                            str(ROOT / 'build/win32/msvc8-cross/package-ci.py'),
                            'calendar', str(obj), str(work)], check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            shutil.rmtree(obj)
            with zipfile.ZipFile(work / 'artifacts/zoolrunner-win32-msvc8-calendar.zip') as archive:
                for name, data in expected.items():
                    with self.subTest(name=name):
                        staged = work / 'runtime' / name
                        self.assertFalse(staged.is_symlink())
                        self.assertEqual(staged.read_bytes(), data)
                        self.assertEqual(archive.read('zoolrunner-calendar/' + name), data)


if __name__ == '__main__':
    unittest.main()
