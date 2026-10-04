#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Verify that formatting enforcement fails with a diff and supports fixing it."""

import pathlib
import stat
import subprocess
import sys
import tempfile


def main():
    formatter, driver = map(pathlib.Path, sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="p4fmt-check-") as directory:
        root = pathlib.Path(directory)
        source = root / "nested" / "file with spaces.p4"
        source.parent.mkdir()
        original = b"control C(){apply{bit<8> x;x=1;}}"
        source.write_bytes(original)
        source.chmod(0o640)
        untouched = root / "not-p4.txt"
        untouched.write_bytes(original)
        command = [sys.executable, driver, "--formatter", formatter]

        check = subprocess.run([*command, "--check", root], capture_output=True)
        assert check.returncode == 1, check.stderr
        assert b"--- a/" in check.stdout and b"+++ b/" in check.stdout
        assert b"@@" in check.stdout
        assert b"-control C(){" in check.stdout and b"+control C() {" in check.stdout
        assert b"\\ No newline at end of file" in check.stdout
        assert source.read_bytes() == original

        subprocess.run([*command, "--write", root], check=True)
        assert source.read_bytes() != original
        assert stat.S_IMODE(source.stat().st_mode) == 0o640
        assert untouched.read_bytes() == original
        check = subprocess.run([*command, "--check", root], capture_output=True)
        assert check.returncode == 0, check.stderr
        assert not check.stdout
        check = subprocess.run(
            [*command, "--check", "--fail-on-fallback", root], capture_output=True
        )
        assert check.returncode == 0, check.stderr
        assert not check.stdout

        # A missing input must fail without modifying other files.
        source.write_bytes(original)
        failed = subprocess.run(
            [*command, "--write", source, root / "missing.p4"], capture_output=True
        )
        assert failed.returncode == 2
        assert source.read_bytes() == original

        # Report every unsupported file and still show diffs for supported input.
        directive = root / "aaa-unsupported.p4"
        directive.write_bytes(b"/* unterminated")
        syntax = root / "zzz-unsupported.p4"
        syntax.write_bytes(b'"unterminated')
        failed = subprocess.run(
            [*command, "--check", "--fail-on-fallback", root], capture_output=True
        )
        assert failed.returncode == 2
        assert b"aaa-unsupported.p4" in failed.stderr
        assert b"zzz-unsupported.p4" in failed.stderr
        assert b"2 of 3 P4 files could not be formatted" in failed.stderr
        assert b"--- a/" in failed.stdout and b"+++ b/" in failed.stdout
        assert b"+control C() {" in failed.stdout
        assert source.read_bytes() == original

        # Strict write mode must not partially update a batch with fallback.
        failed = subprocess.run(
            [*command, "--write", "--fail-on-fallback", root], capture_output=True
        )
        assert failed.returncode == 2
        assert source.read_bytes() == original
        assert directive.read_bytes() == b"/* unterminated"
        assert syntax.read_bytes() == b'"unterminated'


if __name__ == "__main__":
    main()
