#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
#
# SPDX-License-Identifier: Apache-2.0
"""Test argument quoting in the p4c driver."""

import argparse
import os
import shlex
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from p4c_src.driver import BackendDriver, quote  # noqa: E402

PATH_WITH_SPACE = "/tmp/dir with space/sub"
PATH_WITH_QUOTE = "/tmp/it's here/x"


def make_opts(**overrides):
    """Build options for driver tests."""
    opts = dict(
        dry_run=True,
        debug=False,
        output_directory=".",
        source_file="prog.p4",
        run_preprocessor_only=False,
        preprocessor_options=[],
        compiler_options=[],
        Wdisable=None,
        Werror=None,
        debug_info=False,
        assembler_options=[],
        linker_options=[],
        preprocessor_defines=[],
        language="p4-16",
        search_path=[],
        p4runtime_file=None,
        p4runtime_format="binary",
        p4runtime_files=None,
        disabled_annos=None,
        optimizeParserInlining=False,
        log_levels=[],
        passes=[],
        dump_dir=None,
        json=None,
        json_source=None,
        pretty_print=None,
        ndebug_mode=False,
        gdb=False,
        cgdb=False,
        lldb=False,
        skip_preprocessor=False,
        run_till_assembler=False,
        run_all=True,
        inputMetrics=None,
    )
    opts.update(overrides)
    return argparse.Namespace(**opts)


class DriverQuotingTest(unittest.TestCase):
    def setUp(self):
        self._env = dict(os.environ)
        os.environ["P4C_BUILD_TYPE"] = "DEVELOPER"
        os.environ["P4C_16_INCLUDE_PATH"] = "/opt/p4 include/p4include"
        os.environ["P4C_14_INCLUDE_PATH"] = "/opt/p4 include/p4_14include"

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._env)

    def driver(self, **opts):
        drv = BackendDriver("test", "arch")
        drv.add_command("preprocessor", "cc")
        drv.add_command("compiler", "p4c-test")
        drv.process_command_line_options(make_opts(**opts))
        return drv

    @staticmethod
    def args(drv, cmd):
        """Split a command as runCmd does."""
        return shlex.split(" ".join(drv._commands[cmd]))

    def assert_option_value(self, drv, cmd, flag, value):
        argv = self.args(drv, cmd)
        self.assertIn(flag, argv, argv)
        self.assertEqual(value, argv[argv.index(flag) + 1], argv)

    def test_quote_is_noop_for_safe_values(self):
        for safe in ["foo", "a/b.c", "X=1", "--W=a,b", "-I"]:
            self.assertEqual(safe, quote(safe))

    def test_quote_roundtrip(self):
        for value in [PATH_WITH_SPACE, PATH_WITH_QUOTE, 'a"b', "a b'c d", "$HOME", "x;y"]:
            self.assertEqual([value], shlex.split(quote(value)))

    def test_search_path_with_spaces(self):
        drv = self.driver(search_path=[PATH_WITH_SPACE, PATH_WITH_QUOTE])
        for cmd in ("preprocessor", "compiler"):
            argv = self.args(drv, cmd)
            self.assertIn(PATH_WITH_SPACE, argv)
            self.assertIn(PATH_WITH_QUOTE, argv)

    def test_default_include_path_with_spaces(self):
        drv = self.driver()
        for cmd in ("preprocessor", "compiler"):
            self.assert_option_value(drv, cmd, "-I", "/opt/p4 include/p4include")

    def test_p4_14_include_path_with_spaces(self):
        drv = self.driver(language="p4-14")
        self.assert_option_value(drv, "compiler", "-I", "/opt/p4 include/p4_14include")

    def test_defines_with_spaces(self):
        drv = self.driver(preprocessor_defines=["NAME=a value", "PLAIN=1"])
        for cmd in ("preprocessor", "compiler"):
            argv = self.args(drv, cmd)
            self.assertIn("-DNAME=a value", argv)
            self.assertIn("-DPLAIN=1", argv)

    def test_compiler_file_options_with_spaces(self):
        drv = self.driver(
            p4runtime_files=PATH_WITH_SPACE + "/p4info.txt",
            dump_dir=PATH_WITH_SPACE,
            json=PATH_WITH_SPACE + "/ir.json",
            json_source=PATH_WITH_SPACE + "/in.json",
            pretty_print=PATH_WITH_SPACE + "/pp.p4",
            inputMetrics=PATH_WITH_SPACE + "/metrics.json",
            disabled_annos="a b",
        )
        self.assert_option_value(
            drv, "compiler", "--p4runtime-files", PATH_WITH_SPACE + "/p4info.txt"
        )
        self.assert_option_value(drv, "compiler", "--dump", PATH_WITH_SPACE)
        self.assert_option_value(drv, "compiler", "--toJSON", PATH_WITH_SPACE + "/ir.json")
        self.assert_option_value(drv, "compiler", "--fromJSON", PATH_WITH_SPACE + "/in.json")
        self.assert_option_value(drv, "compiler", "--pp", PATH_WITH_SPACE + "/pp.p4")
        argv = self.args(drv, "compiler")
        self.assertIn("--metrics=" + PATH_WITH_SPACE + "/metrics.json", argv)
        self.assertIn("--disable-annotations=a b", argv)

    def test_deprecated_p4runtime_file_with_spaces(self):
        drv = self.driver(p4runtime_file=PATH_WITH_SPACE + "/p4info.txt")
        self.assert_option_value(
            drv, "compiler", "--p4runtime-file", PATH_WITH_SPACE + "/p4info.txt"
        )

    def test_conflicting_debuggers_report_an_error(self):
        # This used to raise AttributeError: BackendDriver had no exitWithError().
        drv = BackendDriver("test", "arch")
        drv.add_command("preprocessor", "cc")
        drv.add_command("compiler", "p4c-test")
        with self.assertRaises(SystemExit) as ctx:
            drv.process_command_line_options(make_opts(gdb=True, lldb=True))
        self.assertEqual(1, ctx.exception.code)

    def test_exit_with_error_prints_message(self):
        drv = BackendDriver("test", "arch")
        import contextlib
        import io

        err = io.StringIO()
        with contextlib.redirect_stderr(err), self.assertRaises(SystemExit) as ctx:
            drv.exitWithError("something went wrong")
        self.assertEqual(1, ctx.exception.code)
        self.assertIn("something went wrong", err.getvalue())


if __name__ == "__main__":
    unittest.main()
