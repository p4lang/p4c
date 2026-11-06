# Structured compiler diagnostics

Language servers and other tools need diagnostic data without interpreting terminal
formatting. p4c provides owned C++ diagnostic records and an optional
[SARIF 2.1.0 plus Errata 01](https://docs.oasis-open.org/sarif/sarif/v2.1.0/sarif-v2.1.0.html)
output adapter. This is a foundation for LSP integration; it is not an LSP server.

```sh
p4test --diagnostics-file diagnostics.sarif program.p4
p4c-bm2-ss --diagnostics-file diagnostics.sarif -o program.json program.p4
p4c --target bmv2 --arch v1model --diagnostics-file diagnostics.sarif program.p4
```

The option is optional. Compiler terminal rendering, warning actions and exit
status conventions remain in use. Without collection or the option, structured
extraction, GCC JSON capture and SARIF writes are disabled. Source-mapping fixes
also apply to normal diagnostics: escaped filenames, number-only line directives,
and the first line of inputs parsed without a preceding preprocessor line marker.

## C++ API and ownership

`lib/diagnostic.h` defines `Diagnostic`, `DiagnosticLocation`, `DiagnosticRange`,
`DiagnosticPosition`, severity and related information. Messages, catalog names,
original file identities, escaped URIs and positions are owned values. Records
retain no IR nodes, source buffers or compilation-context pointers. The `source`
field distinguishes `p4c`, `parser` and `preprocessor` diagnostics.

Enable collection before reporting or invoking the compiler:

```cpp
using Context = P4::P4CContextWithOptions<P4::CompilerOptions>;
std::vector<P4::Diagnostic> diagnostics;
{
    P4::AutoCompileContext context(std::make_unique<Context>());
    auto &reporter = Context::get().errorReporter();
    reporter.enableDiagnosticCollection();
    P4::parseP4String("editor-buffer.p4", 1, "/*é😀*/ bad;");
    diagnostics = reporter.getDiagnostics();
}
// diagnostics still owns its messages, locations and ranges here.
```

The complete, compiled [example](examples/diagnostics.cpp) demonstrates this API
and a future LSP position mapping. `parseP4String` parses an in-memory document;
normal frontend/backend passes can follow using the same context and reporter.
`parseP4File` also captures GCC diagnostics when collection has been enabled.
No SARIF file is required by either API path.

`getDiagnostics()` returns an owned copy in reporting order. An optional
`setDiagnosticConsumer(std::function<void(const Diagnostic &)>)` enables collection
and observes each stored record synchronously. Callback references are borrowed
for the call; copy the record to retain it. Callbacks must not reenter the same
reporter. Caller exceptions propagate after the report write. Setting an empty
consumer detaches the callback without clearing the collection.

Copying an `ErrorReporter` copies its owned diagnostic state and legacy policy/
counters. Consumers and SARIF file sinks are detached in the copy, so a copied
reporter cannot accidentally write into the original compilation's report.

Use a fresh compilation context for each compilation. This isolates diagnostics,
warning policy, counters and deduplication. Enabling collection does not reset a
reporter; reusing a context across translation units retains its state. Context
stack/concurrent compiler access retains p4c's existing threading constraints.
The API does not make the compiler thread-safe.

Compiler diagnostics pass through existing suppression, promotion and
deduplication before snapshotting. Parser errors enter the same collection.
`reportDiagnostic()` imports already-produced external diagnostics, prints/counts
them, and publishes the same record. External producers apply their own warning
policy: GCC warnings are not retroactively changed by p4c's catalog pragmas.
Use GCC warning flags when configuring preprocessor warning actions.

## Source positions and LSP

Canonical positions have **zero-based lines and UTF-16 code-unit columns**.
Range ends are exclusive. A document can be known without a range, and a line can
be known without a column. No default line or column is invented. Primary and end
locations are mapped independently through preprocessor/P4 line directives. An
end in another file or before the start is omitted. Source text is consulted
while extracting a record, so later buffer destruction does not affect it.

The compiler's `SourcePosition` instead uses physical input lines and byte
columns. Conversion counts supplementary Unicode characters as two UTF-16 units
and tabs as one unit; it does not use display-cell widths. Invalid UTF-8, offsets
inside a character and unavailable source text do not produce a guessed column.
As with existing compiler locations, macro expansion does not provide a full
token-level mapping back to macro arguments in the original file.

SARIF serialization adds one to known lines/columns and declares
`columnKind: utf16CodeUnits`. GCC JSON's inclusive finish is converted to an
exclusive boundary first. URIs percent-encode UTF-8 bytes, spaces, quotes, percent
signs and other reserved characters. Disk paths are made absolute and normalized
lexically at reporting time, without resolving symlinks. Parser pseudo-files
such as `<stdin>` remain escaped relative identities.
The `-` stdin identity is also kept relative; ordinary filenames beginning with
parentheses or angle brackets retain normal file URI handling.

A future [LSP adapter](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/)
can group records by `location.uri`, map error/warning/note to LSP severities,
copy message/code and related information, and use known canonical ranges for a
client negotiating UTF-16. A client negotiating UTF-8 or UTF-32 needs conversion
against its document text. Handle missing locations explicitly; never treat
SARIF's one-based fields as LSP positions. LSP clients may also need their own URI
mapping for virtual/unsaved documents. Publish replacement arrays for each affected
document, including empty arrays to clear diagnostics from the previous
compilation. The compiler collection itself contains only the current context.

## Preprocessor and driver boundaries

With collection enabled, the C++ preprocessing path requests
[GCC diagnostic JSON](https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Message-Formatting-Options.html).
Separate temporary files receive stdout and stderr; the process is reaped before
diagnostic decoding and parsing. Large output cannot fill an undrained pipe.
GCC warnings, errors, missing includes, available ranges and child notes are
converted into the canonical model. Preprocess-only mode is supported. GCC
sometimes groups a later warning or fatal error under an earlier diagnostic;
these become separate records in producer order, while child notes become related
information. Valid parent diagnostics survive malformed child records. GCC must
support `-fdiagnostics-format=json` and the byte-column options. Unsupported tools,
malformed records and unexpected stderr are visible diagnostic errors, not silent
losses. In structured mode, preprocessor terminal messages are rendered from JSON;
the original GCC caret layout is not retained. Without collection the original
preprocessing path remains in use.

The Python `p4c` driver is a separate process boundary. Its
`p4c_src.diagnostics` module represents preprocessing records with the same logical
fields; `decode_gcc(record)` returns owned Python diagnostic records without file
output. The driver uses the same
fields and coordinates, converts them to SARIF, and combines them with the child
compiler's SARIF results in process order. Its temporary stderr file is separate
from preprocessor stdout. Earlier warnings survive a later compiler failure.
Subprocess failures retain their nonzero status; a reporting failure turns an
otherwise successful invocation into failure. Driver input errors after report
initialization also produce a report.

Python-captured subprocess diagnostics are **not** automatically accessible via
the C++ API. An embedded C++ caller obtains diagnostics produced inside its own
context, including its own `parseP4File` preprocessing. A driver integration
consumes the aggregate SARIF file or uses Python's `DiagnosticReport` object.
Arbitrary external backends must support `--diagnostics-file` to contribute
compiler records; failure to produce supported SARIF is reported explicitly.
The merger accepts p4c's self-contained results, not general SARIF reports with
artifact/rule index tables. Such unsupported reports produce a visible error.

## Report lifecycle and limits

The compiler and driver initialize complete JSON before ordinary option/input
validation and rewrite complete reports as diagnostics arrive. Compilation-scoped
data is owned by the reporter, not the serializer. Ordinary returns and caught
compiler exceptions leave the last complete report. Report creation, flush and
close failures are visible and cause compilation failure. Disk/permission failures
can prevent a complete write; no successful report is claimed in that case.

Crashes, forced termination and interruptions during an actual file write cannot
guarantee report completion. Third-party backend text and messages written directly
to stderr outside the diagnostic system are not parsed or claimed as structured
compiler diagnostics. Initialization cannot capture failures before it occurs.
Locations already embedded in a diagnostic's suffix text remain part of its
message; related information comes from the reporter's structured source locations.
GCC machine diagnostics are decoded without introducing a new JSON/LSP dependency.

## Tests

`StructuredDiagnostics.*` and `StructuredParserDiagnostics.*` GTests exercise the
API without a SARIF file, lifetimes, policies, source ranges and subprocess capture.
`diagnostic-api-example` compiles and runs the documented embedding example.
`driver_diagnostics` tests the Python boundary. `structured-diagnostics` runs a
fresh 27-group end-to-end acceptance suite when p4test and BMv2 are configured.
Its per-invocation inputs, commands, stdout, stderr, statuses and SARIF reports
remain under `Testing/StructuredDiagnostics`. Official schema validation can be
run externally against the official [Errata 01 schema](https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json)
using JSON Schema Draft 4 and a format checker; no schema download is required
to run the compiler or the configured tests.
External validation needs `jsonschema[format]`; the validator rejects missing
URI/date-time validators rather than silently skipping their format checks.
