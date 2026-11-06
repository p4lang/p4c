// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "frontends/common/preprocessorDiagnostics.h"

#include <unistd.h>

#include <sys/wait.h>

#include <cerrno>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "ir/json_parser.h"
#include "lib/error_reporter.h"

namespace P4 {
namespace {

const JsonData *member(const JsonObject &object, const char *key) {
    auto it = object.find(key);
    return it == object.end() ? nullptr : it->second.get();
}

template <typename T>
const T &required(const JsonData *value) {
    if (!value || !value->is<T>()) throw std::runtime_error("missing or invalid GCC JSON field");
    return *value->to<T>();
}

std::string text(const JsonObject &object, const char *key) {
    return required<JsonString>(member(object, key));
}

unsigned number(const JsonObject &object, const char *key) {
    const auto &value = required<JsonNumber>(member(object, key)).val;
    if (value < 0 || value > std::numeric_limits<unsigned>::max())
        throw std::runtime_error("GCC position outside supported range");
    return static_cast<unsigned>(value);
}

std::optional<std::string> sourceLine(const std::string &file, unsigned line) {
    std::ifstream source(file);
    std::string text;
    for (unsigned i = 0; i < line; ++i)
        if (!std::getline(source, text)) return std::nullopt;
    return text;
}

DiagnosticPosition position(const JsonObject &object, bool inclusiveEnd) {
    auto line = number(object, "line");
    // GCC always supplies byte-column on supported versions. The explicitly
    // requested byte column-unit also makes column a safe fallback.
    const auto &rawColumn =
        required<JsonNumber>(
            member(object, member(object, "byte-column") ? "byte-column" : "column"))
            .val;
    if (rawColumn < -1 || rawColumn > std::numeric_limits<unsigned>::max())
        throw std::runtime_error("GCC column outside supported range");
    auto column = rawColumn > 0 ? static_cast<unsigned>(rawColumn) : 0;
    std::optional<unsigned> converted;
    if (line && column) {
        if (auto bytes = sourceLine(text(object, "file"), line))
            converted = diagnosticUtf16Column(*bytes, column - (inclusiveEnd ? 0 : 1));
    }
    return {line ? line - 1 : 0, converted};
}

DiagnosticLocation location(const JsonObject &object) {
    const auto *start = member(object, "start");
    if (!start) start = member(object, "caret");
    const auto &begin = required<JsonObject>(start);
    auto file = text(begin, "file");
    DiagnosticLocation result{file, diagnosticFileUri(file), std::nullopt};
    if (number(begin, "line")) {
        result.range = DiagnosticRange{position(begin, false), std::nullopt};
        if (auto *finish = member(object, "finish")) {
            const auto &end = required<JsonObject>(finish);
            if (text(end, "file") == file && number(end, "line")) {
                auto pos = position(end, true);
                if (pos.line > result.range->start.line ||
                    (pos.line == result.range->start.line && pos.column &&
                     result.range->start.column && *pos.column >= *result.range->start.column))
                    result.range->end = pos;
            }
        }
    }
    return result;
}

Diagnostic decode(const JsonObject &object) {
    auto kind = text(object, "kind");
    auto severity = DiagnosticSeverity::Note;
    if (kind == "error" || kind == "fatal error")
        severity = DiagnosticSeverity::Error;
    else if (kind == "warning")
        severity = DiagnosticSeverity::Warning;
    else if (kind != "note")
        throw std::runtime_error("unsupported GCC diagnostic kind: " + kind);
    Diagnostic result{severity,       "preprocessor", text(object, "message"),
                      "preprocessor", std::nullopt,   {}};
    if (auto *option = member(object, "option")) result.code = required<JsonString>(option);
    const auto &locations = required<JsonVector>(member(object, "locations"));
    for (size_t i = 0; i < locations.size(); ++i) {
        auto loc = location(required<JsonObject>(locations[i].get()));
        if (i == 0)
            result.location = std::move(loc);
        else
            result.related.push_back({std::move(loc), "Related location"});
    }
    return result;
}

void decodeGroup(const JsonObject &object, std::vector<Diagnostic> &records,
                 std::optional<size_t> parent = std::nullopt) {
    auto diagnostic = decode(object);
    if (diagnostic.severity == DiagnosticSeverity::Note && parent) {
        records[*parent].related.push_back(
            {diagnostic.location.value_or(DiagnosticLocation{}), diagnostic.message});
        records[*parent].related.insert(records[*parent].related.end(), diagnostic.related.begin(),
                                        diagnostic.related.end());
    } else {
        records.push_back(std::move(diagnostic));
        parent = records.size() - 1;
    }
    // GCC can group subsequent warnings and fatal errors under an earlier
    // warning. Those remain independent diagnostics; only notes are related info.
    // Indices stay valid as the vector grows, unlike pointers to its elements.
    if (auto *children = member(object, "children"))
        for (const auto &child : required<JsonVector>(children))
            decodeGroup(required<JsonObject>(child.get()), records, parent);
}

void failure(ErrorReporter &reporter, const std::string &message) {
    reporter.reportDiagnostic({DiagnosticSeverity::Error,
                               "preprocessor-diagnostics",
                               message,
                               "preprocessor",
                               std::nullopt,
                               {}});
}

}  // namespace

bool reportPreprocessorDiagnostics(std::string_view json, ErrorReporter &reporter) {
    std::vector<Diagnostic> records;
    std::optional<std::string> decodingError;
    try {
        if (!diagnosticUtf16Column(json, json.size()))
            throw std::runtime_error("invalid UTF-8 in GCC diagnostic JSON");
        std::istringstream input{std::string(json)};
        std::unique_ptr<JsonData> root;
        parseJson(input, root, true);
        if (!input || !root) throw std::runtime_error("expected a GCC JSON diagnostic array");
        for (const auto &record : required<JsonVector>(root.get()))
            decodeGroup(required<JsonObject>(record.get()), records);
        input >> std::ws;
        if (!input.eof()) throw std::runtime_error("unexpected text after GCC JSON diagnostics");
    } catch (const std::exception &error) {
        decodingError = std::string("Cannot decode GCC preprocessor diagnostics: ") + error.what() +
                        "\n" + std::string(json);
    }
    // Keep valid records preceding a malformed child, and do not misclassify
    // observer/reporting exceptions as decoding failures.
    for (auto &record : records) reporter.reportDiagnostic(std::move(record));
    if (decodingError) {
        failure(reporter, *decodingError);
        return false;
    }
    return true;
}

PreprocessorOutput preprocessWithDiagnostics(const std::string &command, ErrorReporter &reporter) {
    PreprocessorOutput output(std::tmpfile(), &std::fclose);
    PreprocessorOutput errors(std::tmpfile(), &std::fclose);
    if (!output || !errors) {
        failure(reporter, "Cannot create temporary files for preprocessor output");
        return {nullptr, &std::fclose};
    }
    auto child = fork();
    if (child == 0) {
        if (dup2(fileno(output.get()), STDOUT_FILENO) < 0 ||
            dup2(fileno(errors.get()), STDERR_FILENO) < 0)
            _exit(127);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }
    if (child < 0) {
        failure(reporter, "Cannot start preprocessor");
        return {nullptr, &std::fclose};
    }
    int status;
    int waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
        failure(reporter, "Cannot wait for preprocessor");
        return {nullptr, &std::fclose};
    }
    std::rewind(errors.get());
    std::string json;
    char buffer[4096];
    while (auto count = std::fread(buffer, 1, sizeof(buffer), errors.get()))
        json.append(buffer, count);
    if (std::ferror(errors.get())) {
        failure(reporter, "Cannot read preprocessor diagnostics");
        return {nullptr, &std::fclose};
    }
    bool decoded = reportPreprocessorDiagnostics(json, reporter);
    if (status != 0) {
        reporter.diagnose(DiagnosticAction::Error, "io",
                          "Preprocessor returned exit code %d; aborting compilation", "", status);
        return {nullptr, &std::fclose};
    }
    if (!decoded) return {nullptr, &std::fclose};
    std::rewind(output.get());
    return output;
}

}  // namespace P4
