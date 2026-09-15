#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
# SPDX-License-Identifier: Apache-2.0

import os
import stat
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

# Add driver source directory to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import p4c_src.util as util  # noqa: E402


def make_executable(path: Path) -> None:
    path.write_text("#!/bin/sh\n")
    path.chmod(path.stat().st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)


class FindBinTest(unittest.TestCase):
    def setUp(self):
        self._tmp = TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)
        self._old_path = os.environ.get("PATH", "")
        self.addCleanup(lambda: os.environ.__setitem__("PATH", self._old_path))

    def test_finds_executable_directly_on_path(self):
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        make_executable(bin_dir / "p4test")

        os.environ["PATH"] = str(bin_dir)

        self.assertEqual(util.find_bin("p4test"), bin_dir / "p4test")

    def test_does_not_match_file_in_a_subdirectory(self):
        # Subdirectories in PATH entries must be ignored
        bin_dir = self.root / "bin"
        nested = bin_dir / "share" / "examples"
        nested.mkdir(parents=True)
        make_executable(nested / "p4test")

        os.environ["PATH"] = str(bin_dir)

        self.assertIsNone(util.find_bin("p4test"))

    def test_skips_non_executable_match(self):
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        (bin_dir / "p4test").write_text("not actually runnable")

        os.environ["PATH"] = str(bin_dir)

        self.assertIsNone(util.find_bin("p4test"))

    def test_falls_through_to_later_path_entry(self):
        empty_dir = self.root / "empty"
        bin_dir = self.root / "bin"
        empty_dir.mkdir()
        bin_dir.mkdir()
        make_executable(bin_dir / "p4test")

        os.environ["PATH"] = os.pathsep.join([str(empty_dir), str(bin_dir)])

        self.assertEqual(util.find_bin("p4test"), bin_dir / "p4test")


if __name__ == "__main__":
    unittest.main()
