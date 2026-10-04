<!--!
\page p4fmt p4fmt (P4 Formatter)                                                               
-->
<!-- 
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.

Refer to the full page here: [p4fmt (P4 Formatter)](https://p4lang.github.io/p4c/p4fmt.html)
-->
<!--!
\internal
-->
# p4fmt (P4 Formatter)
<!--!
\endinternal
-->
<!--!
[TOC]
-->
p4fmt is a WIP formatter for P4. It's in a highly experimental phase
and, not yet stable/reliable for general use.
Contributions and feedbacks from the community
would be highly appreciated.

## Build
- Setup P4C correctly, from [here](https://github.com/p4lang/p4c#dependencies).

- Follow the instructions here to [build](https://github.com/p4lang/p4c#installing-p4c-from-source).

Later `p4fmt` executable can be found inside the `p4c/build/` dir, and can be invoked as `./build/p4fmt` from p4c's root dir.

## Usage
- Takes an output file with a `-o` flag and writes to it.

    `./build/p4fmt <p4 source file> -o <output_file>`

 - If an output file is not provided with `-o` flag, it just prints the output on the stdout.

    `./build/p4fmt <p4 source file>`

Sample Usage:

    ./build/p4fmtsample.p4
    ./build/p4fmt sample.p4 -o out.p4

## Rejecting fallback

    ./build/p4fmt --fail-on-fallback input.p4

By default, unsupported input is returned unchanged. With `--fail-on-fallback`,
p4fmt instead exits unsuccessfully and reports the filename and reason, including
incomplete tokens or an unsuccessful source-preservation check. No output is emitted and an existing `-o`
file is left untouched. `checkfmt` accepts the same option.

Already-formatted files, empty files, and supported comment-only files still
succeed. This option detects fallback, not whether the output changed.

## Source preservation

p4fmt reads original source without running the C preprocessor. It first tries
the compiler parser and `P4::ToP4` IR printer. When that representation cannot
reproduce the tokens, a tolerant source parser builds a layout document for
`ToP4::printSource`. This handles unresolved types, macros, P4-14, negative tests,
and source literals/parentheses that the compiler IR normalizes.

- `formatTokens.h/.cpp` scans logical text after C line splicing and maps every
  token back to its original physical bytes. Strings and comments stay intact,
  including comments formed across splices.
- `preprocessor.h/.cpp` recognizes directives and newline-terminated pragmas.
  Includes, macro definitions, unknown directives, and continued directive bodies
  are opaque source regions. No conditions are evaluated. Macro arguments retain
  their internal trivia because stringification makes whitespace significant.
- `sourceParser.h/.cpp` recognizes delimiters and statement boundaries. Conditional
  branches start from the same entry context; only compatible delimiter frames
  survive the merge. Incomplete syntax recovers to matching delimiters without
  repairing the program. Strings, comments, and protected regions borrow the
  original buffer through the source layout document.
- `syntaxRecovery.h/.cpp` reconciles compiler-IR output with source spellings;
  `p4fmt.cpp` selects a representation and checks the resulting token boundaries.
  Genuine failures preserve the whole input by default and fail in strict mode.

The source parser deliberately does not implement macro expansion or duplicate
the compiler grammar. Formatting every branch requires retaining source that
cannot be represented by a single preprocessed compiler IR. Directives and macro
bodies are preserved, not reformatted. Comments and their adjacent whitespace
are also retained. This conservative policy can leave local regions unchanged
while formatting the surrounding P4; strict mode rejects whole-file fallback,
not these explicitly modeled regions. This is not a syntax validator.

Both representations use `ToP4` and `SourceCodeBuilder`. `ToP4FormattingOptions`
provides shared indentation width and compiler-IR field/key alignment options.
These C++ options are not yet p4fmt command-line flags. Future style choices
should extend the shared printer rather than copy its IR visitors into p4fmt.

Run its focused tests with:

    cmake --build build --target p4fmt checkfmt p4fmt-source-tests
    ctest --test-dir build --output-on-failure -R '^p4fmt-source-'

The token tests require `ENABLE_GTESTS`.

The separate corpus regression test discovers every `*.p4` file recursively under `testdata`, including
P4-14, negative tests, target extensions, and generated references. It checks
strict successful execution, preservation of non-whitespace content and complete
comments/strings, and idempotence. It never overwrites the inputs. Its report
counts changed and unchanged files separately.

    ctest --test-dir build --output-on-failure -R '^p4fmt-corpus$'

The per-suite report is written to `build/backends/p4fmt/corpus-report.json`.

## Reference Checker for P4Fmt

Sample Usage:
    `./build/checkfmt --file <p4 source file> --reference-file <p4 reference file>`
