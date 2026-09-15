#!/usr/bin/env python3
"""Check archive field boundaries that the original OS X tar misreads."""
import io
from pathlib import Path
import sys
import tarfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from legacy_tar import LegacyTarInfo


class LegacyArchiveTests(unittest.TestCase):
    def test_boundary_round_trip(self):
        for name in ("p/" + "a" * 97, "p/" + "a" * 98,
                     "p/" + "a" * 99, "q/" * 50 + "p/" + "a" * 98,
                     "p/" + "a" * 97 + "/"):
            with self.subTest(name=name):
                member = LegacyTarInfo(name)
                if name.endswith("/"):
                    member.type = tarfile.DIRTYPE
                header = member.tobuf(tarfile.USTAR_FORMAT)
                self.assertIn(b"\0", header[:100])
                self.assertIn(b"\0", header[345:500])
                with tarfile.open(fileobj=io.BytesIO(header + b"\0" * 1024)) as archive:
                    self.assertEqual(archive.getmembers()[0].name, name.rstrip("/"))

    def test_unrepresentable_basename(self):
        with self.assertRaises(ValueError):
            LegacyTarInfo("a" * 100).tobuf(tarfile.USTAR_FORMAT)


if __name__ == "__main__":
    unittest.main()
