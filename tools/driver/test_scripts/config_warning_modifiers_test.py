#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
# SPDX-License-Identifier: Apache-2.0

import sys
import unittest
from pathlib import Path

# Add driver source directory to PYTHONPATH
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from p4c_src.driver import BackendDriver  # noqa: E402


def run_modifier(driver: BackendDriver, args: list[str], option: str) -> list[str]:
    """Helper to run config_warning_modifiers and extract appended compiler flags."""
    driver.add_command("compiler", "p4c-fake-compiler")
    driver.config_warning_modifiers(args, option)
    return driver._commands["compiler"][1:]


class TestConfigWarningModifiers(unittest.TestCase):
    def setUp(self):
        self.driver = BackendDriver("fake", "fake")

    def test_bare_flag(self):
        result = run_modifier(self.driver, [""], "disable")
        self.assertEqual(result, ["--Wdisable"])

    def test_single_diagnostic(self):
        result = run_modifier(self.driver, ["uninitialized_out_param"], "disable")
        self.assertEqual(result, ["--Wdisable=uninitialized_out_param"])

    def test_comma_separated_diagnostics(self):
        result = run_modifier(self.driver, ["foo,bar"], "error")
        self.assertEqual(result, ["--Werror=foo", "--Werror=bar"])

    def test_bare_takes_precedence_over_named(self):
        # A bare --Wdisable or --Werror should apply globally regardless of order or other flags
        res1 = run_modifier(self.driver, ["", "uninitialized_out_param"], "disable")
        res2 = run_modifier(self.driver, ["uninitialized_out_param", ""], "error")

        self.assertEqual(res1, ["--Wdisable"])
        self.assertEqual(res2, ["--Werror"])


if __name__ == "__main__":
    unittest.main()
