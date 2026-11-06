# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0
"""GCC/driver diagnostic boundary and SARIF output adapter.

The C++ in-process API is independent of this subprocess aggregation. Python
uses the same logical positions: zero-based lines/UTF-16 columns, exclusive ends.
"""

import json
import os
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from urllib.parse import quote


def _invalid_constant(value):
    raise ValueError(f"invalid JSON constant: {value}")


def file_uri(file):
    if not file:
        return ""
    if file in ("-", "(string)", "<stdin>", "<built-in>", "<command-line>"):
        return quote(file, safe="/-._~")
    return Path(os.path.abspath(file)).as_uri()


def utf16_column(text, byte_offset):
    if byte_offset < 0:
        return None
    raw = text.encode("utf-8")
    if byte_offset > len(raw):
        return None
    try:
        return len(raw[:byte_offset].decode("utf-8").encode("utf-16-le")) // 2
    except UnicodeError:
        return None


@dataclass
class Location:
    file: str
    uri: str
    # A missing range means the document is known but its position is not.
    start: tuple | None = None
    end: tuple | None = None

    def sarif(self):
        physical = {"artifactLocation": {"uri": self.uri}}
        if self.start is not None:
            region = {"startLine": self.start[0] + 1}
            if self.start[1] is not None:
                region["startColumn"] = self.start[1] + 1
            if self.end is not None:
                region["endLine"] = self.end[0] + 1
                if self.end[1] is not None:
                    region["endColumn"] = self.end[1] + 1
            physical["region"] = region
        return {"physicalLocation": physical}


@dataclass
class Diagnostic:
    severity: str
    code: str
    message: str
    source: str = "preprocessor"
    location: Location | None = None
    related: list = field(default_factory=list)

    def sarif(self):
        result = {
            "level": self.severity,
            "message": {"text": self.message},
            "properties": {"source": self.source},
        }
        if self.code:
            result["ruleId"] = self.code
        if self.location and self.location.uri:
            result["locations"] = [self.location.sarif()]
        if self.related:
            result["relatedLocations"] = []
            for index, (location, message) in enumerate(self.related, 1):
                item = {"id": index, "message": {"text": message}}
                if location and location.uri:
                    item.update(location.sarif())
                result["relatedLocations"].append(item)
        return result

    def print(self):
        prefix = ""
        if self.location:
            prefix = self.location.file
            if self.location.start is not None:
                line, column = self.location.start
                prefix += f":{line + 1}"
                if column is not None:
                    prefix += f":{column + 1}"
            prefix += ": "
        option = f" [{self.code}]" if self.code else ""
        print(f"{prefix}{self.severity}: {self.message}{option}", file=sys.stderr)


def _string(record, key):
    value = record[key]
    if not isinstance(value, str):
        raise ValueError(f"invalid GCC string field: {key}")
    value.encode("utf-8")  # Reject lone surrogate escapes as malformed records.
    return value


def _number(record, key, minimum=0):
    value = record[key]
    if type(value) is not int or not minimum <= value <= 0xFFFFFFFF:
        raise ValueError(f"invalid GCC position: {key}")
    return value


def _position(record, end=False):
    line = _number(record, "line")
    column = _number(record, "byte-column" if "byte-column" in record else "column", minimum=-1)
    converted = None
    if line and column > 0:
        try:
            with open(_string(record, "file"), encoding="utf-8") as source:
                for _ in range(line):
                    text = next(source)
                converted = utf16_column(text.rstrip("\n"), column - (0 if end else 1))
        except (OSError, UnicodeError, StopIteration):
            pass  # File/line is known; a column cannot be inferred without text.
    return line - 1, converted


def _location(record):
    start = record.get("start", record.get("caret"))
    if not isinstance(start, dict):
        raise ValueError("missing GCC source position")
    file = _string(start, "file")
    location = Location(file, file_uri(file))
    if _number(start, "line"):
        location.start = _position(start)
        end = record.get("finish")
        if end is not None and _string(end, "file") == file and _number(end, "line"):
            finish = _position(end, end=True)
            if finish[0] > location.start[0] or (
                finish[0] == location.start[0]
                and finish[1] is not None
                and location.start[1] is not None
                and finish[1] >= location.start[1]
            ):
                location.end = finish
    return location


def _decode_gcc_record(record):
    kind = _string(record, "kind")
    if kind not in ("error", "fatal error", "warning", "note"):
        raise ValueError(f"unsupported GCC diagnostic kind: {kind}")
    diagnostic = Diagnostic(
        "error" if kind == "fatal error" else kind,
        _string(record, "option") if "option" in record else "preprocessor",
        _string(record, "message"),
    )
    locations = record["locations"]
    if not isinstance(locations, list):
        raise ValueError("invalid GCC locations")
    for index, item in enumerate(locations):
        location = _location(item)
        if index == 0:
            diagnostic.location = location
        else:
            diagnostic.related.append((location, "Related location"))
    return diagnostic


def _decode_gcc_group(record, diagnostics, parent=None):
    diagnostic = _decode_gcc_record(record)
    if diagnostic.severity == "note" and parent is not None:
        diagnostics[parent].related.append((diagnostic.location, diagnostic.message))
        diagnostics[parent].related.extend(diagnostic.related)
    else:
        diagnostics.append(diagnostic)
        parent = len(diagnostics) - 1
    children = record.get("children", [])
    if not isinstance(children, list):
        raise ValueError("invalid GCC children")
    for child in children:
        _decode_gcc_group(child, diagnostics, parent)


def decode_gcc(record):
    """Return owned diagnostics in producer order, with child notes as related info."""
    diagnostics = []
    _decode_gcc_group(record, diagnostics)
    return diagnostics


class DiagnosticReport:
    """One driver invocation. Child SARIF is transport, never the C++ API."""

    def __init__(self, file):
        self.file = file
        self.results = []
        self.write()

    def write(self):
        report = {
            "$schema": "https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json",
            "version": "2.1.0",
            "runs": [
                {
                    "tool": {
                        "driver": {"name": "p4c", "informationUri": "https://github.com/p4lang/p4c"}
                    },
                    "columnKind": "utf16CodeUnits",
                    "results": self.results,
                }
            ],
        }
        document = json.dumps(report, ensure_ascii=False, allow_nan=False, separators=(",", ":"))
        encoded = (document + "\n").encode("utf-8")
        with open(self.file, "wb") as output:
            output.write(encoded)

    def add(self, diagnostic):
        diagnostic.print()
        self.results.append(diagnostic.sarif())
        self.write()

    def error(self, message):
        self.add(Diagnostic("error", "driver-diagnostics", message, "driver"))

    def gcc(self, text):
        decoded = []
        decoding_error = None
        try:
            records = json.loads(text, parse_constant=_invalid_constant)
            if not isinstance(records, list):
                raise ValueError("expected a GCC JSON diagnostic array")
            for record in records:
                _decode_gcc_group(record, decoded)
        except (ValueError, KeyError, TypeError, AttributeError) as error:
            decoding_error = f"Cannot decode GCC preprocessor diagnostics: {error}\n{text}"
        for diagnostic in decoded:
            self.add(diagnostic)
        if decoding_error is not None:
            self.error(decoding_error)
            return False
        return True

    def run(self, step, args, invoke):
        if step == "preprocessor":
            flags = [
                "-fdiagnostics-format=json",
                "-fdiagnostics-color=never",
                "-fdiagnostics-column-unit=byte",
                "-fdiagnostics-column-origin=1",
            ]
            with tempfile.TemporaryFile() as errors:
                rc = invoke(args + flags, stderr=errors)
                errors.seek(0)
                raw = errors.read()
            try:
                try:
                    text = raw.decode("utf-8")
                except UnicodeError as error:
                    self.error(f"Invalid UTF-8 in GCC diagnostics: {error}; {raw!r}")
                    return rc or 1
                decoded = self.gcc(text)
            except OSError as error:
                print(f"Cannot write diagnostics file {self.file}: {error}", file=sys.stderr)
                return rc or 1
            return rc if rc else (0 if decoded else 1)

        with tempfile.TemporaryDirectory(prefix="p4c-diagnostics-") as directory:
            file = Path(directory) / "compiler.sarif"
            rc = invoke(args + ["--diagnostics-file", str(file)])
            try:
                with file.open(encoding="utf-8") as source:
                    report = json.load(source, parse_constant=_invalid_constant)
                # Reject lone surrogate escapes before merging any child data.
                json.dumps(report, ensure_ascii=False, allow_nan=False).encode("utf-8")
                if report["version"] != "2.1.0" or len(report["runs"]) != 1:
                    raise ValueError("expected one SARIF 2.1.0 compiler run")
                run = report["runs"][0]
                if run.get("columnKind") != "utf16CodeUnits" or not isinstance(
                    run["results"], list
                ):
                    raise ValueError("unsupported compiler diagnostic encoding")
                if run["tool"]["driver"]["name"] != "p4c" or run.get("artifacts"):
                    raise ValueError("expected self-contained p4c compiler diagnostics")
                for result in run["results"]:
                    if (
                        not isinstance(result, dict)
                        or result.get("level") not in ("error", "warning", "note")
                        or not isinstance(result.get("message"), dict)
                        or not isinstance(result["message"].get("text"), str)
                        or "ruleIndex" in result
                    ):
                        raise ValueError("malformed or indexed compiler diagnostic")
                    for key in ("locations", "relatedLocations"):
                        for location in result.get(key, []):
                            artifact = location.get("physicalLocation", {}).get(
                                "artifactLocation", {}
                            )
                            if "index" in artifact or "uriBaseId" in artifact:
                                raise ValueError("expected self-contained compiler locations")
            except (
                OSError,
                UnicodeError,
                ValueError,
                KeyError,
                TypeError,
                AttributeError,
            ) as error:
                try:
                    self.error(
                        f"Cannot read compiler diagnostics; backend must support --diagnostics-file: {error}"
                    )
                except OSError as write_error:
                    print(
                        f"Cannot write diagnostics file {self.file}: {write_error}", file=sys.stderr
                    )
                return rc or 1
            self.results.extend(run["results"])
            try:
                self.write()
            except OSError as error:
                print(f"Cannot write diagnostics file {self.file}: {error}", file=sys.stderr)
                return rc or 1
            return rc
