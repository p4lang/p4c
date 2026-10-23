# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import ccache_snapshot  # noqa: E402

RESULT_HEADER = b"\xcc\xac\x01\x00"
MANIFEST_HEADER = b"\xcc\xac\x01\x01"


class CacheEntriesTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.cache = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write(self, name: str, contents: bytes) -> None:
        path = self.cache / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents)

    def test_finds_entries_without_filename_suffix(self) -> None:
        """Entries are found by header, as ccache >= 4.13 names them by hash alone."""
        self.write("9/9/8c4578e895f92a43f61ab0b9690e0d2a2feb4e", RESULT_HEADER + b"data")
        self.write("6/d/b97d7d7119e4cfcb43c9d3b5d05cc31adbcdb4", MANIFEST_HEADER + b"data")
        self.write("9/stats", b"1 2 3\n")
        self.write("9/CACHEDIR.TAG", b"Signature: 8a477f597d28d172789f06886806bc55\n")
        self.write("ccache.conf", b"max_size = 750M\n")

        found = {str(p.relative_to(self.cache)) for p in ccache_snapshot.cache_files(self.cache)}

        self.assertEqual(
            found,
            {
                "9/9/8c4578e895f92a43f61ab0b9690e0d2a2feb4e",
                "6/d/b97d7d7119e4cfcb43c9d3b5d05cc31adbcdb4",
            },
        )

    def test_tells_results_from_manifests(self) -> None:
        """Only result entries count as results, whatever their filename."""
        self.write("a/result", RESULT_HEADER + b"data")
        self.write("b/manifest", MANIFEST_HEADER + b"data")

        self.assertTrue(ccache_snapshot.is_result(self.cache / "a/result"))
        self.assertFalse(ccache_snapshot.is_result(self.cache / "b/manifest"))


if __name__ == "__main__":
    unittest.main()
