#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Check deferred lint discovery and target dependencies without running linters."""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class LintFiles(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.source = Path(self.temp.name) / "source"
        self.build = Path(self.temp.name) / "build"
        self.source.mkdir()
        self.repo = Path(__file__).resolve().parents[2]
        self.cmake = os.environ.get("CMAKE_COMMAND", "cmake")
        self.cpp_files = {"backends/old.cpp", "lib/space name.h", "lib/quote'name.h"}
        self.python_files = {"test/cmake/regression.py", "testdata/old.py", "tools/helper.py"}
        for name in (
            self.cpp_files
            | self.python_files
            | {
                "extensions/custom.def",
                "extensions/custom.py",
                "backends/ebpf/runtime/excluded.cpp",
                "backends/tc/runtime/excluded.py",
                "backends/tofino/third_party/excluded.h",
                "backends/tofino/third_party/excluded.py",
            }
        ):
            self.write(name)
        # Directory symlinks remain supported, including out-of-tree extensions.
        external = Path(self.temp.name) / "external"
        external.mkdir()
        (external / "linked.cpp").touch()
        (self.source / "backends/linked").symlink_to(external, target_is_directory=True)
        self.cpp_files.add("backends/linked/linked.cpp")

        # Simulate an unknown external backend registering files from its own scope.
        self.backend = Path(self.temp.name) / "external-backend"
        self.backend.mkdir()
        for name in ("custom.def", "custom.py", "direct.cpp"):
            (self.backend / name).touch()
        (self.backend / "CMakeLists.txt").write_text("""
add_cpplint_files("${CMAKE_CURRENT_SOURCE_DIR}" "custom.def")
add_clang_format_files("${CMAKE_CURRENT_SOURCE_DIR}" "custom.def")
add_black_files("${CMAKE_CURRENT_SOURCE_DIR}" "custom.py")
set_property(GLOBAL APPEND PROPERTY CPPLINT-files "${CMAKE_CURRENT_SOURCE_DIR}/direct.cpp")
""")

        for name in ("cpplint.py", "fake-clang-format", "fake-black", "fake-isort"):
            script = self.source / "tools" / name
            script.write_text(
                f"#!{sys.executable}\n"
                "import json, sys\n"
                f"with open({str(self.build / 'calls.jsonl')!r}, 'a') as output:\n"
                "    output.write(json.dumps(sys.argv) + '\\n')\n"
            )
            script.chmod(0o755)
        self.write(
            "CMakeLists.txt",
            """
cmake_minimum_required(VERSION 3.16.3)
project(LintFiles NONE)
include("${P4C_REPO}/cmake/P4CUtils.cmake")
set(P4C_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(P4C_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(APPEND "${P4C_BINARY_DIR}/configure-count" "configured\n")
add_cpplint_files("${P4C_SOURCE_DIR}" "extensions/custom.def;lib/space name.h")
add_clang_format_files("${P4C_SOURCE_DIR}" "extensions/custom.def")
add_black_files("${P4C_SOURCE_DIR}" "extensions/custom.py;tools/helper.py")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../external-backend" external-backend)
set(CLANG_FORMAT_CMD "${P4C_SOURCE_DIR}/tools/fake-clang-format")
set(BLACK_CMD "${P4C_SOURCE_DIR}/tools/fake-black")
set(ISORT_CMD "${P4C_SOURCE_DIR}/tools/fake-isort")
include("${P4C_REPO}/cmake/Linters.cmake")
""",
        )

    def write(self, name, content=""):
        path = self.source / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    def run_cmake(self, *args):
        result = subprocess.run([self.cmake, *args], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def check_manifest(self, name, expected):
        files = (self.build / name).read_text().split(";")
        self.assertEqual(set(files), {str(self.source / item) for item in expected})
        self.assertEqual(len(files), len(expected), "Duplicate lint inputs")

    def exercise_targets(self, generator):
        self.run_cmake(
            "-S",
            str(self.source),
            "-B",
            str(self.build),
            "-G",
            generator,
            f"-DP4C_REPO={self.repo}",
        )
        manifests = ("cpplint_files.txt", "clang_format_files.txt", "BLACK_files.txt")
        self.run_cmake("--build", str(self.build))
        for name in manifests:
            self.assertFalse((self.build / name).exists(), "Discovery ran before a lint target")

        targets = (
            "cpplint",
            "cpplint-quiet",
            "clang-format",
            "clang-format-fix-errors",
            "black",
            "black-fix-errors",
            "isort",
            "isort-fix-errors",
        )
        self.run_cmake("--build", str(self.build), "--parallel", "4", "--target", *targets)
        expected_cpp = self.cpp_files | {"extensions/custom.def"}
        expected_cpp.add(str(self.backend / "custom.def"))
        expected_cpplint = expected_cpp | {str(self.backend / "direct.cpp")}
        expected_python = self.python_files | {
            "extensions/custom.py",
            str(self.backend / "custom.py"),
        }
        self.check_manifest(manifests[0], expected_cpplint)
        self.check_manifest(manifests[1], expected_cpp)
        self.check_manifest(manifests[2], expected_python)
        calls = [json.loads(line) for line in (self.build / "calls.jsonl").read_text().splitlines()]
        self.assertEqual(len(calls), len(targets))
        for call in calls:
            tool = Path(call[0]).name
            if tool == "cpplint.py":
                expected = expected_cpplint
            elif tool == "fake-clang-format":
                expected = expected_cpp
            else:
                expected = expected_python
            actual_files = [arg for arg in call[1:] if arg.startswith(self.temp.name + "/")]
            # black's configuration file is an option, not a lint input.
            actual_files = [arg for arg in actual_files if not arg.endswith("/pyproject.toml")]
            self.assertEqual(set(actual_files), {str(self.source / item) for item in expected})

        # Discover additions and removals without asking CMake to configure again.
        self.write("backends/new.cpp")
        self.write("tools/new.py")
        (self.source / "backends/old.cpp").unlink()
        (self.source / "testdata/old.py").unlink()
        # Missing manifests must also be recreated before the linter starts.
        for name in manifests:
            (self.build / name).unlink()
        self.run_cmake(
            "--build", str(self.build), "--parallel", "4", "--target", "cpplint", "black"
        )
        self.check_manifest(
            manifests[0], (expected_cpplint - {"backends/old.cpp"}) | {"backends/new.cpp"}
        )
        self.check_manifest(
            manifests[2], (expected_python - {"testdata/old.py"}) | {"tools/new.py"}
        )
        self.assertEqual((self.build / "configure-count").read_text(), "configured\n")

    @unittest.skipUnless(shutil.which("make"), "make is not installed")
    def test_make(self):
        self.exercise_targets("Unix Makefiles")

    @unittest.skipUnless(shutil.which("ninja"), "ninja is not installed")
    def test_ninja(self):
        self.exercise_targets("Ninja")


if __name__ == "__main__":
    unittest.main()
