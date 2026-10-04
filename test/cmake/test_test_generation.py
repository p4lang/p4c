#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Exercise test registration and wrapper regeneration without building p4c."""

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


class TestGeneration(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.source = Path(self.temp.name) / "source"
        self.build = Path(self.temp.name) / "build"
        self.source.mkdir()
        self.repo = Path(__file__).resolve().parents[2]
        (self.source / "CMakeLists.txt").write_text("""
cmake_minimum_required(VERSION 3.16.3)
project(TestGeneration NONE)
enable_testing()
include("${P4C_REPO}/cmake/P4CUtils.cmake")
set(P4C_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(P4C_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/driver.sh" CACHE STRING "")
set(TEST_ARGS "--initial" CACHE STRING "")
set(CTEST_ARGS "--option='two words'" CACHE STRING "")
set(XFAIL OFF CACHE BOOL "")
set(suite_timeout 17 CACHE STRING "")
p4c_add_tests(suite "${DRIVER}" "*.p4" "" "${TEST_ARGS}")
p4c_add_test_with_args(special "${DRIVER}" ${XFAIL} alias a.p4
                      "${TEST_ARGS}" "${CTEST_ARGS}")
# The writer must preserve shell syntax and CMake-looking text verbatim.
p4c_write_test_script("${CMAKE_CURRENT_BINARY_DIR}/literal script.test"
                     [=[${shell} @DRIVER@ $<CONFIG> "quotes";back\\slash]=])
""")
        self.driver = self.source / "driver.sh"
        self.driver.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
        self.driver.chmod(0o755)
        (self.source / "a.p4").write_text("// first program\n")
        (self.source / "b.p4").write_text("// second program\n")

    def run_cmake(self, *args):
        result = subprocess.run(
            [os.environ.get("CMAKE_COMMAND", "cmake"), *args],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def configure(self, *args):
        self.run_cmake(
            "-S",
            str(self.source),
            "-B",
            str(self.build),
            f"-DP4C_REPO={self.repo}",
            *args,
        )

    def registered_tests(self):
        result = subprocess.run(
            [
                os.environ.get("CTEST_COMMAND", "ctest"),
                "--show-only=json-v1",
            ],
            cwd=self.build,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return {test["name"]: test for test in json.loads(result.stdout)["tests"]}

    def stamp_scripts(self):
        # A fixed old timestamp makes unintended rewrites observable without sleeps.
        scripts = list(self.build.rglob("*.test"))
        for script in scripts:
            os.utime(script, ns=(1_000_000_000, 1_000_000_000))
        return {script: script.stat().st_mtime_ns for script in scripts}

    def assert_stamps(self, stamps):
        for script, stamp in stamps.items():
            self.assertEqual(script.stat().st_mtime_ns, stamp, str(script))

    def test_regeneration(self):
        self.configure()
        tests = self.registered_tests()
        self.assertEqual(set(tests), {"suite/a.p4", "suite/b.p4", "special/alias"})
        wrapper = self.build / "special/a.p4.test"
        self.assertTrue(os.access(wrapper, os.X_OK))
        self.assertEqual(tests["special/alias"]["command"][1:], ["--option=two words"])
        properties = {p["name"]: p["value"] for p in tests["suite/a.p4"]["properties"]}
        self.assertEqual(properties["TIMEOUT"], 17)
        self.assertEqual(properties["LABELS"], ["suite"])
        self.assertEqual(
            (self.build / "literal script.test").read_text(),
            '${shell} @DRIVER@ $<CONFIG> "quotes";back\\slash\n',
        )
        output = subprocess.check_output([str(wrapper), "two words"], text=True)
        self.assertEqual(
            output.splitlines(),
            [str(self.source), "--initial", "two words", str(self.source / "a.p4")],
        )

        stamps = self.stamp_scripts()
        # Check ctime too: a redundant chmod can leave mtime unchanged.
        change_times = {script: script.stat().st_ctime_ns for script in stamps}
        self.configure()
        self.assert_stamps(stamps)
        self.assertEqual({script: script.stat().st_ctime_ns for script in stamps}, change_times)
        self.assertEqual(self.registered_tests(), tests)

        # CTest metadata must be refreshed even if no wrapper needs rewriting.
        self.configure("-DXFAIL=ON", "-DCTEST_ARGS=changed", "-Dsuite_timeout=23")
        self.assert_stamps(stamps)
        tests = self.registered_tests()
        properties = {p["name"]: p["value"] for p in tests["suite/a.p4"]["properties"]}
        self.assertEqual(properties["TIMEOUT"], 23)
        test = tests["special/alias"]
        self.assertEqual(test["command"][1:], ["changed"])
        properties = {p["name"]: p["value"] for p in test["properties"]}
        self.assertTrue(properties["WILL_FAIL"])
        self.assertEqual(properties["LABELS"], ["XFAIL", "special"])

        self.configure("-DTEST_ARGS=--changed")
        self.assertNotEqual(wrapper.stat().st_mtime_ns, stamps[wrapper])
        self.assertIn("--changed", wrapper.read_text())
        replacement = self.source / "replacement.sh"
        shutil.copy2(self.driver, replacement)
        self.configure(f"-DDRIVER={replacement}")
        self.assertIn(str(replacement), wrapper.read_text())

        # P4 and driver contents are read at execution time, not baked into wrappers.
        stamps = self.stamp_scripts()
        (self.source / "a.p4").write_text("// edited program\n")
        replacement.write_text('#!/bin/sh\nprintf "new driver\\n"\n')
        self.configure()
        self.assert_stamps(stamps)
        self.assertEqual(subprocess.check_output([str(wrapper)], text=True), "new driver\n")

        wrapper.unlink()
        self.configure()
        self.assertTrue(os.access(wrapper, os.X_OK))
        (self.source / "b.p4").unlink()
        (self.source / "c.p4").write_text("// new program\n")
        self.configure()
        self.assertEqual(
            set(self.registered_tests()), {"suite/a.p4", "suite/c.p4", "special/alias"}
        )

    def add_testgen_suites(self):
        with (self.source / "CMakeLists.txt").open("a") as cmake:
            cmake.write("""
set(P4TESTGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/testgen")
set(RUNNER_FLAGS "" CACHE STRING "")
foreach(backend bmv2 ebpf pna tofino)
  include("${P4C_REPO}/backends/p4tools/modules/testgen/targets/${backend}/test/TestTemplate.cmake")
  set(variants plain)
  if(backend STREQUAL "bmv2")
    list(APPEND variants ENABLE_RUNNER P416_PTF VALIDATE_PROTOBUF VALIDATE_PROTOBUF_IR
         USE_ASSERT_MODE DISABLE_ASSUME_MODE)
  elseif(backend STREQUAL "ebpf")
    list(APPEND variants ENABLE_RUNNER)
  elseif(backend STREQUAL "pna")
    list(APPEND variants P416_PTF)
  endif()
  foreach(variant IN LISTS variants)
    set(variant_options "")
    if(NOT variant STREQUAL "plain")
      set(variant_options "${variant}")
    endif()
    p4tools_add_test_with_args(${variant_options} ${RUNNER_FLAGS}
      TAG "${backend}-${variant}" DRIVER "${DRIVER}" ALIAS example.p4
      P4TEST "${CMAKE_CURRENT_SOURCE_DIR}/a.p4" TARGET "${backend}" ARCH test
      TEST_ARGS "${TEST_ARGS}" CMAKE_ARGS "${CTEST_ARGS}")
  endforeach()
endforeach()
""")

    def test_template_changes(self):
        # Copy the module so this test can edit its template without touching the repo.
        module = self.source / "cmake"
        module.mkdir()
        for name in ("P4CUtils.cmake", "test-script.in"):
            shutil.copy2(self.repo / "cmake" / name, module / name)
        project = self.source / "CMakeLists.txt"
        project.write_text(project.read_text().replace("${P4C_REPO}/cmake/", "cmake/"))
        self.configure()
        self.configure()  # Exercise the unchanged-output path before editing the template.
        template = module / "test-script.in"
        template.write_text(template.read_text() + "# Updated template\n")
        # The template must remain a configure dependency even when generation was skipped.
        self.run_cmake("--build", str(self.build))
        self.assertTrue(
            (self.build / "special/a.p4.test").read_text().endswith("# Updated template\n")
        )

    def test_testgen_regeneration(self):
        self.add_testgen_suites()
        self.configure()
        wrappers = sorted((self.build / "testgen").rglob("*.test"))
        self.assertEqual(len(wrappers), 12)
        for wrapper in wrappers:
            self.assertTrue(os.access(wrapper, os.X_OK))
            self.assertIn("set -e\n", wrapper.read_text())
        stf = self.build / "testgen/bmv2-ENABLE_RUNNER/example.p4.test"
        self.assertIn("for item in ${stffiles[@]}", stf.read_text())
        self.assertIn("run-bmv2-test.py", stf.read_text())
        ptf = self.build / "testgen/pna-P416_PTF/example.p4.test"
        self.assertIn("run-dpdk-ptf-test.py", ptf.read_text())
        proto = self.build / "testgen/bmv2-VALIDATE_PROTOBUF/example.p4.test"
        self.assertIn("--encode=p4testgen.TestCase", proto.read_text())
        kernel = self.build / "testgen/ebpf-ENABLE_RUNNER/example.p4.test"
        self.assertIn("run-ebpf-test.py", kernel.read_text())

        tests = self.registered_tests()
        stamps = self.stamp_scripts()
        self.configure()
        self.assert_stamps(stamps)
        self.assertEqual(self.registered_tests(), tests)

        # Changing an option must add the runner to an existing wrapper.
        plain = self.build / "testgen/bmv2-plain/example.p4.test"
        self.assertNotIn("run-bmv2-test.py", plain.read_text())
        self.configure("-DRUNNER_FLAGS=ENABLE_RUNNER")
        self.assertIn("run-bmv2-test.py", plain.read_text())
        self.configure("-DTEST_ARGS=--changed")
        for wrapper in wrappers:
            self.assertIn("--changed", wrapper.read_text())
        plain.unlink()
        self.configure()
        self.assertTrue(os.access(plain, os.X_OK))


if __name__ == "__main__":
    unittest.main()
