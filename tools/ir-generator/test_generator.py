#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Regression tests for partitioning IR output and preserving incremental builds."""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

GENERATOR = Path(sys.argv.pop(1)).resolve()


class SplitGeneration(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.output = self.root / "output"
        self.inputs = ["core/nodes.def", "backend/nodes.def"]
        self.write(self.inputs[0], "abstract Base { int value; }\n")
        self.backend = '#include "core/nodes.def"\nclass Derived : Base { int extra; }\n'
        self.write(self.inputs[1], self.backend)

    def write(self, name, contents):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)

    def generate(self, success=True, inputs=None):
        result = subprocess.run(
            [
                str(GENERATOR),
                "-r",
                str(self.root),
                "-d",
                str(self.output),
                *[str(self.root / name) for name in (inputs or self.inputs)],
            ],
            capture_output=True,
            text=True,
        )
        if success:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stderr)
        return result

    def snapshot(self):
        return {
            path.relative_to(self.output).as_posix(): (path.read_bytes(), path.stat().st_mtime_ns)
            for path in self.output.rglob("*")
            if path.is_file()
        }

    def mark_outputs(self):
        # No sleeps or dependency on filesystem timestamp resolution.
        for path in self.output.rglob("*"):
            if path.is_file():
                os.utime(path, ns=(1_000_000_000, 1_000_000_000))
        return self.snapshot()

    def test_partition_and_unchanged_outputs(self):
        self.generate()
        expected = {
            "core/nodes.def.h",
            "core/nodes.def.cpp",
            "backend/nodes.def.h",
            "backend/nodes.def.cpp",
            "ir/ir-generated-common.h",
            "ir/ir-generated.h",
            "ir/ir-generated.cpp",
            "ir/gen-tree-macro.h",
        }
        self.assertEqual(set(self.snapshot()), expected)
        core = (self.output / "core/nodes.def.h").read_text()
        backend = (self.output / "backend/nodes.def.h").read_text()
        self.assertIn("class Base", core)
        self.assertNotIn("class Derived", core)
        self.assertIn('#include "core/nodes.def.h"', backend)
        self.assertIn("class Derived", backend)
        before = self.mark_outputs()
        self.generate()
        self.assertEqual(before, self.snapshot())

    def test_backend_field_does_not_rewrite_core_or_tree(self):
        self.generate()
        before = self.mark_outputs()
        self.write(self.inputs[1], self.backend.replace("int extra;", "int extra; int added;"))
        self.generate()
        changed = {name for name, value in self.snapshot().items() if value != before[name]}
        self.assertEqual(changed, {"backend/nodes.def.h", "backend/nodes.def.cpp"})

    def test_new_node_updates_shared_metadata(self):
        self.generate()
        before = self.mark_outputs()
        self.write(self.inputs[1], self.backend + "class Another : Base {}\n")
        self.generate()
        after = self.snapshot()
        self.assertNotEqual(before["ir/gen-tree-macro.h"], after["ir/gen-tree-macro.h"])
        self.assertNotEqual(before["ir/ir-generated.cpp"], after["ir/ir-generated.cpp"])
        self.assertEqual(before["core/nodes.def.h"], after["core/nodes.def.h"])

    def test_method_body_only_rewrites_implementation(self):
        backend = self.backend.replace("int extra;", "int extra; int score() const { return 1; }")
        self.write(self.inputs[1], backend)
        self.generate()
        before = self.mark_outputs()
        self.write(self.inputs[1], backend.replace("return 1;", "return 2;"))
        self.generate()
        changed = {name for name, value in self.snapshot().items() if value != before[name]}
        self.assertEqual(changed, {"backend/nodes.def.cpp"})

    def test_missing_dependency(self):
        self.write(self.inputs[1], "class Derived : Base {}\n")
        result = self.generate(success=False)
        self.assertIn("missing IR header dependency", result.stderr)
        self.assertFalse(self.output.exists())

    def test_cycle_is_rejected_before_writing(self):
        self.generate()
        before = self.mark_outputs()
        self.write(self.inputs[0], '#include "backend/nodes.def"\nabstract Base { int value; }\n')
        result = self.generate(success=False)
        self.assertIn("Cyclic IR header dependency", result.stderr)
        self.assertEqual(before, self.snapshot())

    def test_implementation_dependency_can_point_back(self):
        self.write(
            self.inputs[0],
            '#include_impl "backend/nodes.def"\nabstract Base { int value; }\n',
        )
        self.generate()
        header = (self.output / "core/nodes.def.h").read_text()
        implementation = (self.output / "core/nodes.def.cpp").read_text()
        self.assertNotIn('"backend/nodes.def.h"', header)
        self.assertIn('#include "backend/nodes.def.h"', implementation)

    def test_cpp_implementation_include(self):
        self.write(self.inputs[1], '#include_impl "lib/hex.h"\n' + self.backend)
        self.generate()
        header = (self.output / "backend/nodes.def.h").read_text()
        implementation = (self.output / "backend/nodes.def.cpp").read_text()
        self.assertNotIn('"lib/hex.h"', header)
        self.assertIn('#include "lib/hex.h"', implementation)

    def test_unknown_input_dependency(self):
        self.write(self.inputs[1], '#include "missing.def"\n' + self.backend)
        self.assertIn("not an input", self.generate(success=False).stderr)

    def test_duplicate_input(self):
        result = self.generate(success=False, inputs=self.inputs + [self.inputs[1]])
        # Parsing may reject duplicate class names before partitioning.
        self.assertNotEqual(result.returncode, 0)

    def test_legacy_generation(self):
        header = self.root / "legacy.h"
        result = subprocess.run(
            [str(GENERATOR), "-o", str(header), *[str(self.root / x) for x in self.inputs]],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("class Base", header.read_text())
        self.assertIn("class Derived", header.read_text())
        self.assertNotIn('#include "core/nodes.def"', header.read_text())


if __name__ == "__main__":
    unittest.main()
