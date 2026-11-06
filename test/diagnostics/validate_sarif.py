# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0
"""Validate retained reports against an externally supplied official schema."""

import argparse
import hashlib
import json
import sys
from pathlib import Path

from jsonschema import Draft7Validator, FormatChecker


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('directories', nargs='+', type=Path)
    options = parser.parse_args()
    raw = options.schema.read_bytes()
    schema = json.loads(raw)
    if schema['$schema'] != 'http://json-schema.org/draft-07/schema#':
        parser.error('expected the official SARIF Draft 7 schema')
    Draft7Validator.check_schema(schema)
    validator = Draft7Validator(schema, format_checker=FormatChecker())
    reports = sorted(
        {file.resolve() for directory in options.directories for file in directory.rglob('*.sarif')}
    )
    results = []
    for report in reports:
        try:
            data = report.read_bytes()
            errors = [str(error) for error in validator.iter_errors(json.loads(data))]
            results.append(
                dict(file=str(report), sha256=hashlib.sha256(data).hexdigest(), errors=errors)
            )
        except (OSError, UnicodeError, ValueError) as error:
            results.append(dict(file=str(report), errors=[str(error)]))
    passed = sum(not result['errors'] for result in results)
    summary = dict(
        schema_sha256=hashlib.sha256(raw).hexdigest(),
        draft=7,
        format_checks=True,
        total=len(results),
        passed=passed,
        failed=len(results) - passed,
        reports=results,
    )
    options.output.write_text(json.dumps(summary, indent=2) + '\n')
    print(f'SARIF schema: {passed}/{len(results)} passed; {len(results) - passed} failed')
    sys.exit(0 if results and passed == len(results) else 1)


if __name__ == '__main__':
    main()
