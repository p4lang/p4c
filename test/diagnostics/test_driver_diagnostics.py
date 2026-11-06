# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

import contextlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from importlib import import_module
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "driver"))
from p4c_src.diagnostics import DiagnosticReport, decode_gcc, file_uri, utf16_column

driver_main = import_module("p4c_src.main")


class DriverDiagnostics(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.report = DiagnosticReport(self.root / "report.sarif")
        self.terminal = io.StringIO()
        self.capture = contextlib.redirect_stderr(self.terminal)
        self.capture.__enter__()
        self.addCleanup(self.capture.__exit__, None, None, None)

    @staticmethod
    def invoke(args, stderr=None):
        return subprocess.run(args, stdout=subprocess.PIPE, stderr=stderr, check=False).returncode

    def test_empty_report_is_valid_and_isolated(self):
        self.assertEqual(
            json.loads(self.report.file.read_text())["$schema"],
            "https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json",
        )
        self.assertEqual(json.loads(self.report.file.read_text())["runs"][0]["results"], [])
        self.report.error("first compilation")
        other = DiagnosticReport(self.root / "other.sarif")
        self.assertEqual(other.results, [])

    def test_driver_io_error_has_its_actual_cause(self):
        file = self.root / "driver-failure.sarif"
        with (
            patch.object(sys, "argv", ["p4c", "--diagnostics-file", str(file)]),
            patch.object(driver_main, "_main", side_effect=OSError("output directory unwritable")),
            self.assertRaises(SystemExit) as status,
        ):
            driver_main.main()
        self.assertEqual(status.exception.code, 1)
        report = json.loads(file.read_text())
        self.assertIn(
            "output directory unwritable", report["runs"][0]["results"][0]["message"]["text"]
        )

    def test_utf16_boundaries(self):
        self.assertEqual(utf16_column("a😀é", 7), 4)
        self.assertEqual(utf16_column("\tX", 1), 1)
        self.assertIsNone(utf16_column("😀", 1))
        self.assertIsNone(utf16_column("abc", 4))
        self.assertIsNone(utf16_column("abc", -1))

    def test_uri_escaping(self):
        uri = file_uri(str(self.root / 'space " % é😀.p4'))
        self.assertIn('space%20%22%20%25%20%C3%A9%F0%9F%98%80.p4', uri)
        self.assertEqual(file_uri("<stdin>"), "%3Cstdin%3E")
        self.assertEqual(file_uri("-"), "-")
        self.assertTrue(file_uri("(real).p4").startswith("file://"))

    def test_owned_gcc_range_and_related_information(self):
        source = self.root / 'a.p4'
        source.write_text("a😀éX", encoding="utf-8")
        start = {"file": str(source), "line": 1, "byte-column": 8}
        end = dict(start, **{"byte-column": 8})
        record = {
            "kind": "warning",
            "message": "message 😀",
            "option": "-Wcpp",
            "locations": [{"caret": start, "finish": end}],
            "children": [{"kind": "note", "message": "related", "locations": [{"caret": start}]}],
        }
        diagnostic = decode_gcc(record)[0]
        self.assertEqual(diagnostic.location.start, (0, 4))
        self.assertEqual(diagnostic.location.end, (0, 5))
        self.assertEqual(diagnostic.related[0][1], "related")
        result = diagnostic.sarif()
        self.assertEqual(result["locations"][0]["physicalLocation"]["region"]["startColumn"], 5)
        self.assertEqual(result["relatedLocations"][0]["message"]["text"], "related")

    def test_unknown_position_has_no_invented_coordinates(self):
        diagnostic = decode_gcc({"kind": "error", "message": "unknown", "locations": []})[0]
        self.assertNotIn("locations", diagnostic.sarif())
        diagnostic = decode_gcc(
            {
                "kind": "error",
                "message": "unknown",
                "locations": [{"caret": {"file": "virtual.p4", "line": 0, "column": 0}}],
            }
        )[0]
        self.assertNotIn("region", diagnostic.sarif()["locations"][0]["physicalLocation"])

    def test_malformed_records_preserve_previous_warning(self):
        records = [{"kind": "warning", "message": "earlier", "locations": []}, {"kind": "unknown"}]
        self.assertFalse(self.report.gcc(json.dumps(records)))
        self.assertEqual(self.report.results[0]["message"]["text"], "earlier")
        self.assertEqual(self.report.results[1]["level"], "error")
        for malformed in ("", "[", "[{}]", "{}", "[] extra", "[{},]", "[1]"):
            with self.subTest(malformed=malformed):
                self.assertFalse(self.report.gcc(malformed))

    def test_non_gcc_configuration_is_visible(self):
        def unsupported(args, stderr=None):
            stderr.write(b"unknown option: -fdiagnostics-format=json\n")
            return 9

        self.assertEqual(self.report.run("preprocessor", ["clang"], unsupported), 9)
        self.assertIn("Cannot decode GCC", self.report.results[0]["message"]["text"])

    def test_grouped_warning_and_error_preserve_severities_and_notes(self):
        record = {
            "kind": "warning",
            "message": "earlier",
            "locations": [],
            "children": [
                {
                    "kind": "fatal error",
                    "message": "missing",
                    "locations": [],
                    "children": [{"kind": "note", "message": "included here", "locations": []}],
                }
            ],
        }
        records = decode_gcc(record)
        self.assertEqual([d.severity for d in records], ["warning", "error"])
        self.assertEqual(records[1].related[0][1], "included here")
        self.assertTrue(self.report.gcc(json.dumps([record])))
        self.assertEqual([r["level"] for r in self.report.results], ["warning", "error"])

    def test_malformed_child_preserves_valid_parent(self):
        record = {
            "kind": "warning",
            "message": "valid parent",
            "locations": [],
            "children": [{"kind": "unknown"}],
        }
        self.assertFalse(self.report.gcc(json.dumps([record])))
        self.assertEqual(self.report.results[0]["message"]["text"], "valid parent")
        self.assertEqual(self.report.results[1]["level"], "error")

    def test_gcc11_fatal_footer_preserves_warning_order_and_failure_status(self):
        payload = (
            json.dumps(
                [
                    {
                        "kind": "warning",
                        "message": "earlier",
                        "locations": [],
                        "children": [
                            {"kind": "fatal error", "message": "missing include", "locations": []}
                        ],
                    }
                ]
            )
            + "\ncompilation terminated.\n"
        )

        def preprocessor(args, stderr=None):
            stderr.write(payload.encode("utf-8"))
            return 7

        self.assertEqual(self.report.run("preprocessor", ["cc", "-E"], preprocessor), 7)
        self.assertEqual([r["level"] for r in self.report.results], ["warning", "error"])
        self.assertEqual(
            [r["message"]["text"] for r in self.report.results], ["earlier", "missing include"]
        )

    def test_unexpected_gcc_trailer_is_visible_without_erasing_warning(self):
        payload = '[{"kind":"warning","message":"earlier","locations":[]}]\nunknown trailer'
        self.assertFalse(self.report.gcc(payload))
        self.assertEqual(self.report.results[0]["level"], "warning")
        self.assertEqual(self.report.results[0]["message"]["text"], "earlier")
        self.assertEqual(self.report.results[1]["ruleId"], "driver-diagnostics")
        self.assertIn("unknown trailer", self.report.results[1]["message"]["text"])

    def test_gcc_fatal_footer_requires_an_error_record(self):
        payload = '[{"kind":"warning","message":"earlier","locations":[]}]\ncompilation terminated.'
        self.assertFalse(self.report.gcc(payload))
        self.assertEqual(self.report.results[0]["level"], "warning")
        self.assertEqual(self.report.results[1]["ruleId"], "driver-diagnostics")

    def test_failed_compiler_preserves_exit_status_and_earlier_warning(self):
        self.report.gcc('[{"kind":"warning","message":"earlier","locations":[]}]')
        self.assertEqual(self.report.run("compiler", ["missing"], lambda args: 7), 7)
        self.assertEqual(self.report.results[0]["message"]["text"], "earlier")
        self.assertIn("backend must support", self.report.results[-1]["message"]["text"])

    def test_successful_unsupported_compiler_is_an_error(self):
        self.assertEqual(self.report.run("compiler", ["third-party"], lambda args: 0), 1)

    def test_compiler_results_are_aggregated_in_order(self):
        self.report.gcc('[{"kind":"warning","message":"earlier","locations":[]}]')

        def compiler(args):
            other = DiagnosticReport(Path(args[-1]))
            other.error("later")
            return 3

        self.assertEqual(self.report.run("compiler", ["compiler"], compiler), 3)
        self.assertEqual([r["message"]["text"] for r in self.report.results], ["earlier", "later"])

    def test_malformed_compiler_results_are_visible(self):
        def compiler(args):
            other = DiagnosticReport(Path(args[-1]))
            other.results = [1]
            other.write()
            return 0

        self.assertEqual(self.report.run("compiler", ["compiler"], compiler), 1)
        self.assertEqual(self.report.results[-1]["level"], "error")

    def test_invalid_compiler_run_and_unicode_retain_child_failure(self):
        surrogate = json.loads(self.report.file.read_text())
        surrogate["runs"][0]["results"] = [{"level": "error", "message": {"text": "\ud800"}}]
        for malformed in (None, [], {"version": "2.1.0", "runs": [None]}, surrogate):
            with self.subTest(malformed=malformed):

                def compiler(args):
                    Path(args[-1]).write_text(json.dumps(malformed))
                    return 7

                self.assertEqual(self.report.run("compiler", ["compiler"], compiler), 7)
                self.assertEqual(self.report.results[-1]["level"], "error")

    def test_indexed_compiler_locations_are_rejected(self):
        for key in ("locations", "relatedLocations"):
            for artifact in ({"index": 0}, {"uri": "input.p4", "uriBaseId": "SOURCE"}):
                with self.subTest(key=key, artifact=artifact):

                    def compiler(args):
                        other = DiagnosticReport(Path(args[-1]))
                        other.results = [
                            {
                                "level": "error",
                                "message": {"text": "indexed"},
                                key: [{"physicalLocation": {"artifactLocation": artifact}}],
                            }
                        ]
                        other.write()
                        return 7

                    self.assertEqual(self.report.run("compiler", ["compiler"], compiler), 7)
                    self.assertEqual(self.report.results[-1]["ruleId"], "driver-diagnostics")

    def test_invalid_gcc_utf8_and_write_failure_retain_child_status(self):
        def preprocessor(args, stderr=None):
            stderr.write(b"\xff")
            return 9

        self.report.file = self.root
        self.assertEqual(self.report.run("preprocessor", ["cpp"], preprocessor), 9)

    def test_large_separate_subprocess_output_does_not_deadlock(self):
        program = (
            "import json,sys; sys.stdout.write('x'*262144); "
            "json.dump([dict(kind='warning',message='y'*262144,locations=[])],sys.stderr)"
        )
        rc = self.report.run("preprocessor", [sys.executable, "-c", program], self.invoke)
        self.assertEqual(rc, 0)
        self.assertEqual(len(self.report.results[0]["message"]["text"]), 262144)

    def test_creation_and_write_errors_are_visible(self):
        with self.assertRaises(OSError):
            DiagnosticReport(self.root / "absent" / "out.sarif")
        self.report.file = self.root
        with self.assertRaises(OSError):
            self.report.write()

    def test_write_failure_retains_nonzero_child_status(self):
        self.report.file = self.root
        self.assertEqual(self.report.run("compiler", ["compiler"], lambda args: 7), 7)

    def test_malformed_unicode_escape_is_visible(self):
        self.assertFalse(self.report.gcc(r'[{"kind":"note","message":"\ud800","locations":[]}]'))
        self.assertEqual(self.report.results[-1]["level"], "error")

    def test_unknown_gcc_column(self):
        diagnostic = decode_gcc(
            {
                "kind": "error",
                "message": "unknown column",
                "locations": [{"caret": {"file": "virtual.p4", "line": 3, "byte-column": -1}}],
            }
        )[0]
        self.assertEqual(diagnostic.location.start, (2, None))
        blank = self.root / "blank.p4"
        blank.write_text("\n")
        diagnostic = decode_gcc(
            {
                "kind": "note",
                "message": "blank line",
                "locations": [{"caret": {"file": str(blank), "line": 1, "byte-column": 1}}],
            }
        )[0]
        self.assertEqual(diagnostic.location.start, (0, 0))

    def test_deterministic_serialization(self):
        self.report.error("stable")
        first = self.report.file.read_bytes()
        self.report.write()
        self.assertEqual(first, self.report.file.read_bytes())


if __name__ == "__main__":
    unittest.main()
