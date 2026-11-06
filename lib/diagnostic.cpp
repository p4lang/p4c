// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "lib/diagnostic.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

#include "lib/error_message.h"

namespace P4 {

std::optional<unsigned> diagnosticUtf16Column(std::string_view line, unsigned byteOffset) {
    if (byteOffset > line.size()) return std::nullopt;
    unsigned column = 0;
    for (unsigned i = 0; i < byteOffset;) {
        auto first = static_cast<unsigned char>(line[i]);
        unsigned size = first < 0x80 ? 1 : first < 0xe0 ? 2 : first < 0xf0 ? 3 : 4;
        if (first >= 0x80 && (first < 0xc2 || first > 0xf4)) return std::nullopt;
        if (i + size > byteOffset) return std::nullopt;
        unsigned codepoint = first & (size == 1 ? 0x7f : (1u << (7 - size)) - 1);
        for (unsigned j = 1; j < size; ++j) {
            auto next = static_cast<unsigned char>(line[i + j]);
            if ((next & 0xc0) != 0x80) return std::nullopt;
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if ((size == 3 && codepoint < 0x800) || (size == 4 && codepoint < 0x10000) ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint > 0x10ffff)
            return std::nullopt;
        column += codepoint > 0xffff ? 2 : 1;
        i += size;
    }
    return column;
}

std::string diagnosticFileUri(std::string_view file) {
    if (file.empty()) return {};
    std::string path(file);
    // Parser pseudo-files identify in-memory documents, not files in the cwd.
    bool virtualFile = file == "-" || file == "(string)" || file == "<stdin>" ||
                       file == "<built-in>" || file == "<command-line>";
    if (!virtualFile) {
        std::error_code error;
        auto absolute = std::filesystem::absolute(path, error);
        if (!error) path = absolute.lexically_normal().generic_string();
    }
    std::string uri = !path.empty() && path.front() == '/' ? "file://" : "";
    constexpr char hex[] = "0123456789ABCDEF";
    for (unsigned char ch : path) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
            ch == '/' || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            uri += ch;
        } else {
            uri += '%';
            uri += hex[ch >> 4];
            uri += hex[ch & 15];
        }
    }
    return uri;
}

namespace {

std::optional<DiagnosticLocation> location(const Util::SourceInfo &source) {
    if (!source.isValid()) {
        if (source.filename.isNullOrEmpty()) return std::nullopt;
        DiagnosticLocation result{source.filename.string(),
                                  diagnosticFileUri(source.filename.string()), std::nullopt};
        // Deserialized IR retains original file/line, but no source buffer from
        // which its byte column could safely be converted to UTF-16.
        if (source.line > 0)
            result.range = DiagnosticRange{{static_cast<unsigned>(source.line - 1), std::nullopt},
                                           std::nullopt};
        return result;
    }
    auto start = source.toPosition();
    auto end = source.toPositionEnd();
    DiagnosticLocation result{start.fileName.string(), diagnosticFileUri(start.fileName.string()),
                              std::nullopt};
    if (start.sourceLine > 0) {
        std::optional<unsigned> startColumn;
        try {
            startColumn =
                diagnosticUtf16Column(source.getStartLine(), source.getStart().getColumnNumber());
        } catch (const std::out_of_range &) {
            // Synthetic ranges may extend beyond the available buffer text.
        }
        result.range = DiagnosticRange{{start.sourceLine - 1, startColumn}, std::nullopt};
        if (end.fileName == start.fileName && end.sourceLine > 0) {
            std::string_view endText;
            try {
                endText = source.getEndLine();
            } catch (const std::out_of_range &) {
                return result;
            }
            DiagnosticPosition finish{
                end.sourceLine - 1,
                diagnosticUtf16Column(endText, source.getEnd().getColumnNumber())};
            if (finish.line > result.range->start.line ||
                (finish.line == result.range->start.line && finish.column &&
                 result.range->start.column && *finish.column >= *result.range->start.column))
                result.range->end = finish;
        }
    }
    return result;
}

}  // namespace

Diagnostic makeDiagnostic(const ErrorMessage &message) {
    auto severity = message.type == ErrorMessage::MessageType::Error ? DiagnosticSeverity::Error
                    : message.type == ErrorMessage::MessageType::Warning
                        ? DiagnosticSeverity::Warning
                        : DiagnosticSeverity::Note;
    Diagnostic result{severity, message.prefix, message.message + message.suffix,
                      "p4c",    std::nullopt,   {}};
    if (!message.locations.empty()) result.location = location(message.locations.front());
    for (size_t i = 1; i < message.locations.size(); ++i) {
        if (auto related = location(message.locations[i]))
            result.related.push_back({std::move(*related), "Related location"});
    }
    return result;
}

Diagnostic makeDiagnostic(const ParserErrorMessage &message) {
    return {DiagnosticSeverity::Error,  "parser", message.message, "parser",
            location(message.location), {}};
}

const char *diagnosticSeverityName(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Note:
            return "note";
    }
    return "note";
}

}  // namespace P4
