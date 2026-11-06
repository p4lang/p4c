# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0
"""Validate retained reports against an externally supplied official schema."""

import argparse
import hashlib
import json
import sys
from pathlib import Path

from jsonschema import Draft4Validator, FormatChecker


def invalid_constant(value):
    raise ValueError(f'Invalid JSON constant: {value}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('directories', nargs='+', type=Path)
    options = parser.parse_args()
    raw = options.schema.read_bytes()
    schema = json.loads(raw)
    if (
        schema.get('$schema') != 'http://json-schema.org/draft-04/schema#'
        or schema.get('id')
        != 'https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json'
    ):
        parser.error('expected the official SARIF Errata 01 Draft 4 schema')
    Draft4Validator.check_schema(schema)
    checker = FormatChecker()
    # jsonschema silently accepts unknown formats when their optional validators
    # are absent. Require the formats used by the official SARIF schema.
    if not {'uri', 'uri-reference', 'date-time'} <= checker.checkers.keys():
        parser.error('SARIF format checking requires jsonschema[format] dependencies')
    validator = Draft4Validator(schema, format_checker=checker)
    reports = sorted(
        {file.resolve() for directory in options.directories for file in directory.rglob('*.sarif')}
    )
    results = []
    for report in reports:
        try:
            data = report.read_bytes()
            document = json.loads(data, parse_constant=invalid_constant)
            errors = [str(error) for error in validator.iter_errors(document)]
            results.append(
                dict(file=str(report), sha256=hashlib.sha256(data).hexdigest(), errors=errors)
            )
        except (OSError, UnicodeError, ValueError) as error:
            results.append(dict(file=str(report), errors=[str(error)]))
    passed = sum(not result['errors'] for result in results)
    summary = dict(
        schema_sha256=hashlib.sha256(raw).hexdigest(),
        draft=4,
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
