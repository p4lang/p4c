// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#ifndef LIB_DIAGNOSTIC_H_
#define LIB_DIAGNOSTIC_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace P4 {

struct ErrorMessage;
struct ParserErrorMessage;

enum class DiagnosticSeverity { Error, Warning, Note };

/// Positions use zero-based lines and UTF-16 code-unit columns. An absent column
/// means only the line is known. These are not SourcePosition's byte offsets.
struct DiagnosticPosition {
    unsigned line = 0;
    std::optional<unsigned> column;
};

struct DiagnosticRange {
    DiagnosticPosition start;
    /// Exclusive end, when known and in the same original document as start.
    std::optional<DiagnosticPosition> end;
};

struct DiagnosticLocation {
    std::string file;  /// Original source identity, including #line mappings.
    std::string uri;   /// Escaped URI, resolved at reporting time.
    std::optional<DiagnosticRange> range;
};

struct DiagnosticRelatedInformation {
    DiagnosticLocation location;
    std::string message;
};

/// An owned snapshot: no IR nodes, SourceInfo, source buffers or context pointers.
/// Copies remain usable after the compiler and its input buffers are destroyed.
struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string message;
    std::string source;  /// p4c, parser, or preprocessor.
    std::optional<DiagnosticLocation> location;
    std::vector<DiagnosticRelatedInformation> related;
};

/// Convert a byte offset at a UTF-8 character boundary into UTF-16 code units.
/// Invalid UTF-8, a split character or an out-of-bounds offset has no column.
std::optional<unsigned> diagnosticUtf16Column(std::string_view line, unsigned byteOffset);
std::string diagnosticFileUri(std::string_view file);
Diagnostic makeDiagnostic(const ErrorMessage &message);
Diagnostic makeDiagnostic(const ParserErrorMessage &message);
const char *diagnosticSeverityName(DiagnosticSeverity severity);

}  // namespace P4

#endif /* LIB_DIAGNOSTIC_H_ */
