#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Exercise formatting, conservative fallback, and exact reference-file I/O."""

import pathlib
import re
import subprocess
import sys
import tempfile


def main():
    formatter, checker = map(pathlib.Path, sys.argv[1:3])
    testdata = pathlib.Path(sys.argv[3])
    with tempfile.TemporaryDirectory(prefix="p4fmt-") as directory:
        root = pathlib.Path(directory)
        source = root / "input.p4"

        def format_source(contents, *, fail_on_fallback=False):
            source.write_bytes(contents)
            command = [formatter]
            if fail_on_fallback:
                command.append("--fail-on-fallback")
            result = subprocess.run([*command, source], capture_output=True, check=True)
            assert not result.stderr, result.stderr
            return result.stdout

        simple = b"control C(){apply{bit<8> x;x=1;}}"
        expected = b"control C() {\n    apply {\n        bit<8> x;\n        x = 1;\n    }\n}\n"
        assert format_source(simple) == expected
        assert format_source(expected) == expected
        assert format_source(simple, fail_on_fallback=True) == expected
        assert format_source(expected, fail_on_fallback=True) == expected

        # Reuse ToP4's support for slices, bare annotations, and width expressions.
        for contents, expected in [
            (b"const bit<4> x=8w255[0+:4];", b"const bit<4> x = 8w255[0+:4];\n"),
            (
                b"@marker control C(){apply{}}",
                b"@marker control C() {\n    apply {\n    }\n}\n",
            ),
            (b"header H{bit<8+8> x;}", b"header H {\n    bit<8 + 8> x;\n}\n"),
        ]:
            assert format_source(contents) == expected
            assert format_source(expected) == expected
            assert format_source(contents, fail_on_fallback=True) == expected

        # Unchanged output is not necessarily fallback: these need no edits.
        for contents in [
            b"",
            b"// comment only, without a newline",
            b"/* block comment only */\r\n",
        ]:
            assert format_source(contents) == contents
            assert format_source(contents, fail_on_fallback=True) == contents

        # Structural formatting covers preprocessing, unresolved names, and
        # incomplete grammar without asking the compiler to repair tokens.
        for contents in [
            b"#include <does-not-exist.p4>\n" + simple,
            b"#if FEATURE\ncontrol C(\n#else\ncontrol D(\n#endif\n){}\n",
            b"#vendor extension\r\n" + simple,
            b"UNKNOWN_MACRO(foo)\n" + simple,
            b"control C( {",
            b"const bit<8> x=0x01;",
            b"const bit<8> x=(1);",
            b"@ pragma(1) control C(){apply{}}",
            b"const bool tmp = (1 != 8w2[0 +: -1]);",
        ]:
            formatted = format_source(contents, fail_on_fallback=True)
            assert formatted != contents, contents
            assert format_source(formatted, fail_on_fallback=True) == formatted, formatted

        unsupported = [b"/* unterminated", b'const string s = "unterminated', b"bad\x00byte"]
        for contents in unsupported:
            assert format_source(contents) == contents, contents
            output = root / "must-not-overwrite.p4"
            output.write_bytes(b"existing output")
            failed = subprocess.run(
                [formatter, "--fail-on-fallback", source, "-o", output], capture_output=True
            )
            assert failed.returncode != 0, contents
            assert not failed.stdout, contents
            assert b"formatting fallback:" in failed.stderr, failed.stderr
            assert str(source).encode() in failed.stderr, failed.stderr
            assert source.read_bytes() == contents
            assert output.read_bytes() == b"existing output"

        # A failed format must not create a requested output file either.
        output = root / "must-not-create.p4"
        failed = subprocess.run(
            [formatter, "--fail-on-fallback", source, "-o", output], capture_output=True
        )
        assert failed.returncode != 0
        assert not output.exists()

        for contents in [
            b"/* #include is a comment */\n" + simple,
            b"// header\n" + simple + b" // EOF",
            b"control C(){apply{ /* empty body */ }}",
            b"control C(){apply{bit<8> x;x=/* value */1; // tail\n}}",
        ]:
            formatted = format_source(contents)
            assert formatted != contents, contents
            assert format_source(formatted) == formatted, formatted
            assert format_source(contents, fail_on_fallback=True) == formatted

        # Preprocessor phase ordering and whitespace-sensitive macro expansion.
        # Compare GCC's expansion in both branches, including variadic arguments,
        # stringification, token pasting, aliases, and continued comments/tokens.
        preprocessing = br"""#define STR(...) #__VA_ARGS__
#define ALIAS STR
#define CAT(a,b) a ## b
#define VALUE \
  0x01
#if FEATURE
control C(){apply{const string s=ALIAS(a+  b);}}
#else
control D(){apply{const string s=STR(a +b, "+");}}
#endif
const bit<8> CAT(fo,o)=VALUE;
const bit<8> spl\
iced=1;
/\
/ comment continued \
with punctuation { }
const bit<8>   after_comment=2;
"""
        formatted = format_source(preprocessing, fail_on_fallback=True)
        assert formatted != preprocessing
        assert format_source(formatted, fail_on_fallback=True) == formatted
        for feature in (0, 1):

            def expand(data):
                return subprocess.run(
                    ["cpp", "-P", "-undef", "-nostdinc", f"-DFEATURE={feature}", "-"],
                    input=data,
                    capture_output=True,
                    check=True,
                ).stdout

            # Preserve strings exactly while ignoring other expanded whitespace.
            def tokens(data):
                return b"".join(re.findall(rb'"(?:\\.|[^"\\])*"|[^\s"]+', data))

            assert tokens(expand(preprocessing)) == tokens(expand(formatted))

        # Moving a stray # to the start of a line would create a null CPP
        # directive and turn this intentionally invalid program into valid P4.
        invalid = b"header H {}\nconst H h = { # };\n"
        formatted = format_source(invalid, fail_on_fallback=True)
        assert b"{ # }" in formatted
        assert (
            b"#"
            in subprocess.run(
                ["cpp", "-P", "-"], input=formatted, capture_output=True, check=True
            ).stdout
        )

        # Initializer braces and specialized calls retain ToP4's inline layout.
        for contents in [
            b"#include <x.p4>\nconst S s = {a = 1,b = 2};\n",
            b"#include <x.p4>\nf<bit<8>>({ 1, 2 });\n",
            b"#include <x.p4>\nconst S s = (S){};\n",
            b"#include <x.p4>\nfor (; i < 10; ) ;\n",
            b"#include <x.p4>\ntransition select (x) {\n{b = 0}: reject;\n}\n",
        ]:
            formatted = format_source(contents, fail_on_fallback=True)
            assert formatted.count(b"\n") == contents.count(b"\n")

        conditional = format_source(
            b"#include <x.p4>\ncontrol C(){apply{if (x == (S){f = 1}) {x = 1;}}}",
            fail_on_fallback=True,
        )
        assert b"(S) { f = 1 }" in conditional
        assert format_source(conditional, fail_on_fallback=True) == conditional

        # Unknown macros can also be supplied by an including translation unit;
        # even the compiler-IR printing path must protect their argument trivia.
        macro = b"control C(){apply{F(a+  b);}}"
        assert b"F(a+  b)" in format_source(macro, fail_on_fallback=True)

        for contents in [
            b"#if X\r\ncontrol C(){\r\n#else\r\ncontrol D(){\r\n#endif\r\napply{}}\r\n",
            b'@pragma name "multiline\nstring"\nconst bit<8> x=0x01;',
            b"#define X 1\\\r\n + 2\r\nconst bit<8>   x=X;",
            b"#if X\ncontrol C(){\n#if Y\napply{\n#else\napply{\n#endif\n}\n#endif\n}",
        ]:
            formatted = format_source(contents, fail_on_fallback=True)
            assert formatted != contents
            assert format_source(formatted, fail_on_fallback=True) == formatted

        sample = testdata / "p4_16_samples" / "p4fmt-comments.p4"
        reference = testdata / "p4fmt_outputs" / "p4fmt-comments.p4"
        formatted = format_source(sample.read_bytes())
        assert formatted == reference.read_bytes()
        assert format_source(formatted) == formatted

        # Reference checking must treat source as bytes, including shell syntax.
        contents = b"#define TEXT $(false) `false` $HOME \\\ntext\n"
        source.write_bytes(contents)
        reference = root / "reference with spaces.p4"
        reference.write_bytes(contents)
        subprocess.run([checker, "--file", source, "--reference-file", reference], check=True)
        subprocess.run(
            [checker, "--file", source, "--reference-file", reference, "--overwrite"],
            check=True,
        )
        assert reference.with_suffix(".ref").read_bytes() == contents
        reference.with_suffix(".ref").write_bytes(b"existing reference")
        source.write_bytes(b"/* unterminated")
        failed = subprocess.run(
            [
                checker,
                "--fail-on-fallback",
                "--file",
                source,
                "--reference-file",
                reference,
                "--overwrite",
            ],
            capture_output=True,
        )
        assert failed.returncode != 0
        assert b"formatting fallback:" in failed.stderr, failed.stderr
        assert reference.with_suffix(".ref").read_bytes() == b"existing reference"
        source.write_bytes(contents)
        reference.write_bytes(contents + b"different")
        result = subprocess.run(
            [checker, "--file", source, "--reference-file", reference], capture_output=True
        )
        assert result.returncode != 0

        result = subprocess.run([formatter, root / "missing.p4"], capture_output=True)
        assert result.returncode != 0
        assert not result.stdout

        result = subprocess.run([formatter, root], capture_output=True)
        assert result.returncode != 0
        assert b"Failed to read" in result.stderr

        # Empty output must also work through -o and the reference checker.
        source.write_bytes(b"")
        output = root / "empty.p4"
        subprocess.run([formatter, source, "-o", output], check=True)
        assert output.read_bytes() == b""
        subprocess.run([checker, "--file", source, "--reference-file", output], check=True)


if __name__ == "__main__":
    main()
