// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "lib/sarif.h"

#include <fstream>
#include <sstream>

namespace P4 {
namespace {

void string(std::ostream &out, const std::string &value) {
    // Diagnostic messages may contain bytes from a malformed input file. JSON
    // text must still be UTF-8; preserve valid text and replace invalid sequences.
    std::string valid;
    for (size_t i = 0; i < value.size();) {
        auto first = static_cast<unsigned char>(value[i]);
        unsigned size = first < 0x80 ? 1 : first < 0xe0 ? 2 : first < 0xf0 ? 3 : 4;
        auto character = std::string_view(value).substr(i, size);
        if (diagnosticUtf16Column(character, size)) {
            valid += character;
            i += size;
        } else {
            valid += "\xef\xbf\xbd";
            ++i;
        }
    }
    // Diagnostic strings are length-delimited and may contain embedded NULs.
    // Keep them out of cstring, whose size() uses strlen().
    out << '"';
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char ch : valid) {
        if (ch == '"' || ch == '\\') {
            out << '\\' << ch;
        } else if (ch < 0x20) {
            out << "\\u00" << hex[ch >> 4] << hex[ch & 15];
        } else {
            out << ch;
        }
    }
    out << '"';
}

void location(std::ostream &out, const DiagnosticLocation &loc) {
    out << "\"physicalLocation\":{\"artifactLocation\":{\"uri\":";
    string(out, loc.uri);
    out << '}';
    if (loc.range) {
        const auto &range = *loc.range;
        out << ",\"region\":{\"startLine\":" << range.start.line + 1;
        if (range.start.column) out << ",\"startColumn\":" << *range.start.column + 1;
        if (range.end) {
            out << ",\"endLine\":" << range.end->line + 1;
            if (range.end->column) out << ",\"endColumn\":" << *range.end->column + 1;
        }
        out << '}';
    }
    out << '}';
}

}  // namespace

void writeSarif(std::ostream &out, const std::vector<Diagnostic> &diagnostics) {
    out << R"({"$schema":"https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json",)"
           R"("version":"2.1.0","runs":[{"tool":{"driver":{"name":"p4c",)"
           R"("informationUri":"https://github.com/p4lang/p4c"}},)"
           R"("columnKind":"utf16CodeUnits","results":[)";
    bool first = true;
    for (const auto &diagnostic : diagnostics) {
        if (!first) out << ',';
        first = false;
        out << "{\"level\":";
        string(out, diagnosticSeverityName(diagnostic.severity));
        if (!diagnostic.code.empty()) {
            out << ",\"ruleId\":";
            string(out, diagnostic.code);
        }
        out << ",\"message\":{\"text\":";
        string(out, diagnostic.message);
        out << "},\"properties\":{\"source\":";
        string(out, diagnostic.source);
        out << '}';
        if (diagnostic.location && !diagnostic.location->uri.empty()) {
            out << ",\"locations\":[{";
            location(out, *diagnostic.location);
            out << "}]";
        }
        if (!diagnostic.related.empty()) {
            out << ",\"relatedLocations\":[";
            unsigned id = 0;
            for (const auto &related : diagnostic.related) {
                if (id) out << ',';
                out << "{\"id\":" << ++id << ",\"message\":{\"text\":";
                string(out, related.message);
                out << '}';
                if (!related.location.uri.empty()) {
                    out << ',';
                    location(out, related.location);
                }
                out << '}';
            }
            out << ']';
        }
        out << '}';
    }
    out << "]}]}\n";
}

bool SarifWriter::write(const std::vector<Diagnostic> &diagnostics) const {
    // Serialize before touching the previous report, so a formatting/allocation
    // exception leaves the last complete document intact.
    std::ostringstream serialized;
    writeSarif(serialized, diagnostics);
    auto document = serialized.str();
    // A fresh stream also detects report paths which become unwritable after
    // initialization. Flush and close errors must be visible to the reporter.
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(document.data(), document.size());
    out.flush();
    bool success = out.good();
    out.close();
    return success && !out.fail();
}

}  // namespace P4
