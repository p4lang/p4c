#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Regression tests for clang-tidy coverage selection and failure handling."""

import argparse
import importlib.util
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("tidy", Path(__file__).with_name("run-clang-tidy.py"))
tidy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tidy)


class SelectionTests(unittest.TestCase):
    def test_source_change_selects_only_changed_translation_unit(self):
        self.assertEqual(
            tidy.select_units(["ir/a.cpp", "lib/b.cpp"], {"ir/a.cpp": [[4, 8]]}, "", "", 0, 1),
            ["ir/a.cpp"],
        )

    def test_header_change_keeps_all_possible_consumers(self):
        units = ["backends/bmv2/a.cpp", "frontends/b.cpp", "ir/c.cpp"]
        for suffix in (".h", ".hpp", ".def", ".ypp", ".l", ".ll"):
            with self.subTest(suffix=suffix):
                self.assertEqual(
                    tidy.select_units(units, {"ir/node" + suffix: [[1, 2]]}, "", "", 0, 1), units
                )

    def test_shards_cover_every_selected_file_exactly_once(self):
        units = [f"ir/file{i}.cpp" for i in range(17)]
        shards = [tidy.select_units(units, None, "", "", i, 4) for i in range(4)]
        flattened = [name for shard in shards for name in shard]
        self.assertCountEqual(flattened, units)
        self.assertEqual(len(flattened), len(set(flattened)))

    def test_backend_profiles_do_not_overlap(self):
        prefixes = ("backends/tofino/", "backends/p4tools/modules/testgen/targets/tofino/")
        units = [
            "backends/bmv2/a.cpp",
            "backends/tofino/a.cpp",
            "backends/p4tools/modules/testgen/targets/tofino/a.cpp",
            "ir/node.cpp",
        ]
        portable = tidy.select_units(units, None, "", prefixes, 0, 1)
        tofino = tidy.select_units(units, None, prefixes, "", 0, 1)
        self.assertFalse(set(portable) & set(tofino))
        self.assertCountEqual(portable + tofino, units)

    def test_compilation_database_resolves_paths_and_deduplicates(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            entries = [
                {"directory": directory, "file": name, "command": "clang++ -c " + name}
                for name in (
                    "ir/node.cpp",
                    "ir/node.cpp",
                    "build/_deps/dep.cpp",
                    "ir/../ir/other.cpp",
                )
            ]
            (root / "compile_commands.json").write_text(json.dumps(entries))
            with patch.object(tidy, "SOURCE_ROOT", root):
                self.assertEqual(tidy.translation_units(root), ["ir/node.cpp", "ir/other.cpp"])

    def test_unity_database_does_not_silently_pass_with_zero_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "compile_commands.json").write_text(
                json.dumps(
                    [
                        {
                            "directory": directory,
                            "file": "CMakeFiles/ir.dir/Unity/unity_0_cxx.cxx",
                        },
                        {"directory": directory, "file": "ir/standalone.cpp"},
                    ]
                )
            )
            with (
                patch.object(tidy, "SOURCE_ROOT", root),
                self.assertRaisesRegex(ValueError, "non-unity"),
            ):
                tidy.translation_units(root)

    def test_generated_parsers_are_analyzed_for_grammar_edits(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build = root / "build"
            build.mkdir()
            parser = build / "frontends/parsers/p4/p4parser.cpp"
            (build / "compile_commands.json").write_text(
                json.dumps(
                    [
                        {"directory": str(build), "file": str(parser)},
                        {
                            "directory": str(build),
                            "file": str(build / "_deps/protobuf/generated.cpp"),
                        },
                    ]
                )
            )
            with patch.object(tidy, "SOURCE_ROOT", root):
                units = tidy.translation_units(build)
            self.assertEqual(units, [str(parser)])
            self.assertEqual(
                tidy.select_units(
                    units,
                    {"frontends/parsers/p4/p4parser.ypp": [[1, 2]]},
                    "frontends/",
                    "",
                    0,
                    1,
                    build,
                ),
                units,
            )

    def test_diff_handles_spaces_and_deletion_only_changes(self):
        names = "ir/with spaces.cpp\0ir/node.h\0README.md\0"
        diffs = iter([names, "@@ -3,0 +4,2 @@\n+x\n+y\n", "@@ -8,3 +7,0 @@\n-x\n-y\n-z\n"])
        with patch.object(tidy, "git", side_effect=lambda *args: next(diffs)):
            self.assertEqual(
                tidy.changed_lines("main"), {"ir/with spaces.cpp": [[4, 5]], "ir/node.h": []}
            )

    def test_deleted_header_checks_consumers_but_deleted_source_is_not_compiled(self):
        names = "ir/removed.cpp\0ir/removed.h\0"
        diff = "deleted file mode 100644\n@@ -1,3 +0,0 @@\n-x\n-y\n-z\n"
        responses = iter([names, diff, diff])
        with patch.object(tidy, "git", side_effect=lambda *args: next(responses)):
            changes = tidy.changed_lines("main")
        self.assertEqual(changes, {"ir/removed.h": []})
        self.assertEqual(
            tidy.select_units(["ir/consumer.cpp"], changes, "", "", 0, 1),
            ["ir/consumer.cpp"],
        )


class ExecutionTests(unittest.TestCase):
    def setUp(self):
        self.args = argparse.Namespace(
            clang_tidy="clang-tidy",
            build_dir=Path("build"),
            base="main",
            staged=False,
            profile=False,
            fix=False,
            timeout=1,
        )

    def test_timeout_fails_instead_of_looking_like_clean_analysis(self):
        with patch.object(tidy.subprocess, "run", side_effect=subprocess.TimeoutExpired("tidy", 1)):
            result = tidy.analyze("ir/node.cpp", self.args, "[]")
        self.assertEqual(result["returncode"], 124)
        self.assertIn("incomplete", result["output"])

    def test_missing_tool_fails(self):
        with patch.object(tidy.subprocess, "run", side_effect=FileNotFoundError("clang-tidy")):
            self.assertNotEqual(tidy.analyze("ir/node.cpp", self.args, "[]")["returncode"], 0)

    def test_compiler_errors_are_propagated(self):
        completed = subprocess.CompletedProcess([], 1, "", "fatal error: missing header")
        with patch.object(tidy.subprocess, "run", return_value=completed) as run:
            result = tidy.analyze(
                "ir/node.cpp", self.args, '[{"name":"ir/node.cpp","lines":[[1,2]]}]'
            )
        self.assertEqual(result["returncode"], 1)
        self.assertIn("--warnings-as-errors=*", run.call_args.args[0])
        self.assertIn("missing header", result["output"])


CLANG_TIDY = os.environ.get("CLANG_TIDY") or shutil.which("clang-tidy")


@unittest.skipUnless(CLANG_TIDY, "clang-tidy is not installed")
class AnalyzerIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "ir").mkdir()
        self.source = self.root / "ir/leak.cpp"
        self.source.write_text(
            "void leak() {\n  int *p = new int(1);\n  (void)p;\n}\nint clean() { return 0; }\n"
        )
        (self.root / ".clang-tidy").write_text(
            "Checks: '-*,clang-analyzer-cplusplus.NewDeleteLeaks'\n"
        )
        (self.root / "compile_commands.json").write_text(
            json.dumps(
                [
                    {
                        "directory": str(self.root),
                        "file": str(self.source),
                        "arguments": ["clang++", "-std=c++20", "-c", str(self.source)],
                    }
                ]
            )
        )
        self.args = argparse.Namespace(
            clang_tidy=CLANG_TIDY,
            build_dir=self.root,
            base="main",
            staged=False,
            profile=False,
            fix=False,
            timeout=30,
        )

    def analyze(self, line_filter):
        with patch.object(tidy, "SOURCE_ROOT", self.root):
            return tidy.analyze("ir/leak.cpp", self.args, json.dumps(line_filter))

    def test_new_leak_fails(self):
        result = self.analyze([{"name": str(self.source), "lines": [[1, 4]]}])
        self.assertNotEqual(result["returncode"], 0)
        self.assertIn("clang-analyzer-cplusplus.NewDeleteLeaks", result["output"])

    def test_unchanged_leak_is_filtered(self):
        result = self.analyze([{"name": str(self.source), "lines": [[5, 5]]}])
        self.assertEqual(result["returncode"], 0, result["output"])

    def test_deletion_only_filter_is_valid_and_filters_warnings(self):
        result = self.analyze([{"name": str(self.root / ".clang-tidy-no-changed-lines")}])
        self.assertEqual(result["returncode"], 0, result["output"])

    def test_unfiltered_parse_error_still_fails(self):
        self.source.write_text("#include <missing-header.h>\nint clean() { return 0; }\n")
        result = self.analyze([{"name": str(self.source), "lines": [[2, 2]]}])
        self.assertNotEqual(result["returncode"], 0)
        self.assertIn("missing-header.h", result["output"])

    def test_changed_header_leak_is_not_hidden(self):
        header = self.root / "ir/leak.h"
        header.write_text(self.source.read_text())
        self.source.write_text('#include "leak.h"\nvoid run() { leak(); }\n')
        result = self.analyze([{"name": str(header), "lines": [[1, 4]]}])
        self.assertNotEqual(result["returncode"], 0)
        self.assertIn("clang-analyzer-cplusplus.NewDeleteLeaks", result["output"])


if __name__ == "__main__":
    unittest.main()
