# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0
"""Fresh acceptance suite for #4122. Logs and inputs are retained per run."""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from urllib.parse import quote

BUILD = SOURCE = EVIDENCE = None


class DiagnosticsAcceptance(unittest.TestCase):
    def setUp(self):
        self.directory = EVIDENCE / self._testMethodName
        self.directory.mkdir()
        self.index = 0

    def source(self, text, name="input.p4"):
        path = self.directory / name
        path.write_text(text, encoding="utf-8")
        return path

    def invoke(self, *args, tool="p4test", report=True):
        self.index += 1
        prefix = self.directory / str(self.index)
        file = prefix.with_suffix(".sarif")
        command = [str(BUILD / tool)]
        if report:
            command += ["--diagnostics-file", str(file)]
        command += [str(a) for a in args]
        env = os.environ.copy()
        env.pop("P4TEST_REPLACE", None)
        run = subprocess.run(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, timeout=120
        )
        prefix.with_suffix(".stdout").write_bytes(run.stdout)
        prefix.with_suffix(".stderr").write_bytes(run.stderr)
        prefix.with_suffix(".command.json").write_text(json.dumps(command, indent=2) + "\n")
        prefix.with_suffix(".exit").write_text(str(run.returncode) + "\n")
        if not report:
            return run, None
        self.assertTrue(file.exists(), f"No SARIF report: {run.stderr.decode(errors='replace')}")
        document = json.loads(file.read_text(encoding="utf-8"))
        self.assertEqual(
            document["$schema"],
            "https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json",
        )
        self.assertEqual(document["version"], "2.1.0")
        self.assertEqual(document["runs"][0]["columnKind"], "utf16CodeUnits")
        return run, document["runs"][0]["results"]

    def parse(self, text, *args, name="input.p4"):
        return self.invoke("--parse-only", *args, self.source(text, name))

    def warning_sample(self, *args):
        return self.invoke(*args, SOURCE / "testdata/p4_16_samples/pragma-deprecated.p4")

    def test_01_optional_empty_report(self):
        run, records = self.parse("struct S { bit<8> x; }")
        self.assertEqual(run.returncode, 0)
        self.assertEqual(records, [])

    def test_02_compiler_error(self):
        run, records = self.invoke(self.source("const bit<8> x = true;"))
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(
            any(r["level"] == "error" and r["properties"]["source"] == "p4c" for r in records)
        )

    def test_03_parser_error(self):
        run, records = self.parse("this is not P4;")
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(records[0]["ruleId"], "parser")
        self.assertIn("syntax error", records[0]["message"]["text"])

    def test_04_warning_codes(self):
        run, records = self.warning_sample()
        self.assertEqual(run.returncode, 0)
        self.assertTrue(
            any(r["ruleId"] == "deprecated" and r["level"] == "warning" for r in records)
        )

    def test_05_suppression(self):
        run, records = self.warning_sample("--Wdisable")
        self.assertEqual(run.returncode, 0)
        self.assertFalse(any(r["level"] == "warning" for r in records))

    def test_06_promotion(self):
        run, records = self.warning_sample("--Werror=deprecated")
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(any(r["ruleId"] == "deprecated" and r["level"] == "error" for r in records))

    def test_07_related_locations(self):
        _, records = self.warning_sample()
        self.assertTrue(
            any(r.get("relatedLocations") for r in records if r["ruleId"] == "deprecated")
        )

    def test_08_gcc_warning(self):
        run, records = self.parse('#warning "earlier"\nstruct S { bit<8> x; }')
        self.assertEqual(run.returncode, 0)
        self.assertEqual(records[0]["properties"]["source"], "preprocessor")
        self.assertEqual(records[0]["level"], "warning")
        self.assertIn("earlier", records[0]["message"]["text"])
        text = '#warning "é😀"\nstruct S { bit<8> x; }'
        run, records = self.parse(text, name="unicode.p4")
        self.assertEqual(run.returncode, 0)
        region = records[0]["locations"][0]["physicalLocation"]["region"]
        start = len(text[: text.index("warning")].encode("utf-16-le")) // 2 + 1
        self.assertEqual(region["startColumn"], start)
        self.assertEqual(region["endColumn"], start + len("warning"))
        self.assertIn("é😀", records[0]["message"]["text"])

    def test_09_gcc_error(self):
        run, records = self.parse('#error "stop"\n')
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(
            any("stop" in r["message"]["text"] and r["level"] == "error" for r in records)
        )

    def test_10_missing_include(self):
        include = '#include "é😀missing_4122.p4"'
        run, records = self.parse('#warning "earlier"\n' + include + '\n')
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(any("missing_4122.p4" in r["message"]["text"] for r in records))
        self.assertEqual(records[0]["level"], "warning")
        missing = next(r for r in records if "No such file" in r["message"]["text"])
        region = missing["locations"][0]["physicalLocation"]["region"]
        self.assertEqual(region["startLine"], 2)
        self.assertEqual(region["endColumn"], len(include.encode("utf-16-le")) // 2 + 1)

    def test_11_preprocessor_warning_before_compiler_failure(self):
        run, records = self.invoke(self.source('#warning "earlier"\nconst bit<8> x = true;'))
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(records[0]["level"], "warning")
        self.assertTrue(
            any(r["level"] == "error" and r["properties"]["source"] == "p4c" for r in records[1:])
        )

    def test_12_nested_include_mapping(self):
        self.source('#include "inner.p4"\n', "outer.p4")
        inner = self.source('this is not P4;\n', "inner.p4")
        _, records = self.parse('#include "outer.p4"\n')
        location = records[0]["locations"][0]["physicalLocation"]
        self.assertEqual(location["artifactLocation"]["uri"], inner.as_uri())
        self.assertEqual(location["region"]["startLine"], 1)

    def test_13_line_directive(self):
        _, records = self.parse('#line 44 "mapped.p4"\nthis is not P4;\n', "--nocpp")
        location = records[0]["locations"][0]["physicalLocation"]
        self.assertTrue(location["artifactLocation"]["uri"].endswith("/mapped.p4"))
        self.assertEqual(location["region"]["startLine"], 44)
        _, records = self.parse('#line 44\nthis is not P4;\n', "--nocpp", name="number.p4")
        self.assertEqual(records[0]["locations"][0]["physicalLocation"]["region"]["startLine"], 44)

    def test_14_utf16_columns_and_exclusive_end(self):
        text = '/*é😀*/ true;\n'
        _, records = self.parse(text, "--nocpp")
        region = records[0]["locations"][0]["physicalLocation"]["region"]
        start = len(text[: text.index("true")].encode("utf-16-le")) // 2 + 1
        self.assertEqual(region["startColumn"], start)
        self.assertEqual(region["endColumn"], start + len("true"))

    def test_15_escaped_file_uri(self):
        name = "space ' \" % é😀.p4"
        _, records = self.parse('#warning "path"\nbad;\n', name=name)
        uri = records[0]["locations"][0]["physicalLocation"]["artifactLocation"]["uri"]
        self.assertTrue(uri.endswith(quote(name, safe="/-._~")), uri)
        parser_records = [r for r in records if r["properties"]["source"] == "parser"]
        self.assertTrue(parser_records)
        self.assertEqual(
            parser_records[0]["locations"][0]["physicalLocation"]["artifactLocation"]["uri"], uri
        )

    def test_16_p4_14_original_source(self):
        path = self.source('header_type H { fields { x : ; } }\n', "v1 space ' \" % é😀.p4")
        run, records = self.invoke('--std', 'p4-14', '--parse-only', path)
        self.assertNotEqual(run.returncode, 0)
        configuration = (BUILD / "CMakeCache.txt").read_text()
        if "SUPPORT_P4_14:BOOL=OFF" in configuration:
            self.assertTrue(
                any(
                    record.get("ruleId") == "invalid"
                    and record["level"] == "error"
                    and "Illegal language version p4-14" in record["message"]["text"]
                    for record in records
                )
            )
            return
        locations = [loc for r in records for loc in r.get("locations", [])]
        self.assertTrue(
            any(
                loc["physicalLocation"]["artifactLocation"]["uri"] == path.as_uri()
                for loc in locations
            )
        )

    def test_17_deterministic_reports(self):
        path = self.source('bad;\n')
        self.invoke('--parse-only', path)
        self.invoke('--parse-only', path)
        self.assertEqual(
            (self.directory / '1.sarif').read_bytes(), (self.directory / '2.sarif').read_bytes()
        )

    def test_18_compilation_isolation(self):
        _, first = self.parse('bad;\n')
        _, second = self.parse('struct S { bit<8> x; }', name='good.p4')
        self.assertTrue(first)
        self.assertEqual(second, [])

    def test_19_early_invalid_option(self):
        file = self.directory / "early.sarif"
        run, _ = self.invoke('--no-such-option-4122', '--diagnostics-file', file, report=False)
        self.assertNotEqual(run.returncode, 0)
        records = json.loads(file.read_text())["runs"][0]["results"]
        self.assertTrue(any("Unknown option" in r["message"]["text"] for r in records))

    def test_20_report_creation_failure(self):
        run, _ = self.invoke(
            '--diagnostics-file', self.directory / 'absent/out.sarif', report=False
        )
        self.assertNotEqual(run.returncode, 0)
        self.assertIn(b'Cannot write diagnostics file', run.stderr)

    def test_21_report_write_failure(self):
        target = '/dev/full' if Path('/dev/full').exists() else self.directory
        run, _ = self.invoke('--diagnostics-file', target, report=False)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn(b'Cannot write diagnostics file', run.stderr)

    def test_22_preprocess_only(self):
        run, records = self.invoke(
            '-E', self.source('#warning "earlier"\nstruct S { bit<8> x; }\n')
        )
        self.assertEqual(run.returncode, 0)
        self.assertIn(b'struct S', run.stdout)
        self.assertTrue(any(r["level"] == "warning" for r in records))

    def test_23_exit_status_preservation(self):
        for index, text in enumerate(('bad;', '#error stop\n', 'struct S { bit<8> x; }')):
            path = self.source(text, f'case{index}.p4')
            plain, _ = self.invoke('--parse-only', path, report=False)
            reported, _ = self.invoke('--parse-only', path)
            self.assertEqual(plain.returncode, reported.returncode)

    def test_24_compiler_stderr_compatibility(self):
        source = SOURCE / 'testdata/p4_16_samples/pragma-deprecated.p4'
        plain, _ = self.invoke('--nocpp', source, report=False)
        reported, _ = self.invoke('--nocpp', source)
        self.assertEqual(plain.returncode, reported.returncode)
        self.assertEqual(plain.stderr, reported.stderr)

    def test_25_bmv2_json_and_p4info_compatibility(self):
        source = SOURCE / 'testdata/p4_16_samples/issue1520-bmv2.p4'
        json_file = self.directory / 'output.json'
        p4info = self.directory / 'p4info.txtpb'
        args = ['--p4runtime-files', p4info, '-o', json_file, source]
        plain, _ = self.invoke(*args, tool='p4c-bm2-ss', report=False)
        self.assertEqual(plain.returncode, 0, plain.stderr)
        first_json, first_p4info = json_file.read_bytes(), p4info.read_bytes()
        reported, _ = self.invoke(*args, tool='p4c-bm2-ss')
        self.assertEqual(reported.returncode, 0, reported.stderr)
        self.assertEqual(first_json, json_file.read_bytes())
        self.assertEqual(first_p4info, p4info.read_bytes())
        self.assertEqual(plain.stderr, reported.stderr)

    def test_26_driver_aggregation_and_preprocess_only(self):
        path = self.source('#warning "earlier"\nconst bit<8> x = true;')
        run, records = self.invoke(
            '--target', 'bmv2', '--arch', 'v1model', '-o', self.directory, path, tool='p4c'
        )
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(records[0]["level"], "warning")
        self.assertTrue(
            any(r["properties"]["source"] == 'p4c' and r["level"] == 'error' for r in records)
        )
        run, records = self.invoke(
            '--target', 'bmv2', '--arch', 'v1model', '-E', '-o', self.directory, path, tool='p4c'
        )
        self.assertEqual(run.returncode, 0)
        self.assertTrue(any(r["level"] == 'warning' for r in records))
        missing = self.source(
            '#warning "before missing"\n#include "missing_4122.p4"\n', 'missing.p4'
        )
        run, records = self.invoke(
            '--target', 'bmv2', '--arch', 'v1model', '-o', self.directory, missing, tool='p4c'
        )
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(any('missing_4122.p4' in r['message']['text'] for r in records))
        self.assertEqual(records[0]['level'], 'warning')

    def test_27_driver_early_failure(self):
        run, records = self.invoke(
            '--target', 'bmv2', '--arch', 'v1model', self.directory / 'missing.p4', tool='p4c'
        )
        self.assertNotEqual(run.returncode, 0)
        self.assertTrue(any('does not exist' in r['message']['text'] for r in records))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--source-dir', type=Path, required=True)
    parser.add_argument('--evidence-dir', type=Path, required=True)
    parser.add_argument('--test', help='Run one named acceptance group')
    options = parser.parse_args()
    BUILD, SOURCE = options.build_dir.resolve(), options.source_dir.resolve()
    options.evidence_dir.mkdir(parents=True, exist_ok=True)
    EVIDENCE = Path(tempfile.mkdtemp(prefix='run-', dir=options.evidence_dir))
    print(f'Evidence: {EVIDENCE}', flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(
        unittest.TestSuite([DiagnosticsAcceptance(options.test)])
        if options.test
        else unittest.defaultTestLoader.loadTestsFromTestCase(DiagnosticsAcceptance)
    )
    summary = dict(
        tests=result.testsRun,
        failures=len(result.failures),
        errors=len(result.errors),
        skipped=len(result.skipped),
    )
    (EVIDENCE / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    sys.exit(0 if result.wasSuccessful() else 1)
