// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "lib/error_reporter.h"

#include <optional>

#include "lib/sarif.h"

namespace P4 {

ErrorReporter::ErrorReporter()
    : infoCount(0),
      warningCount(0),
      errorCount(0),
      maxErrorCount(20),
      outputstream(&std::cerr),
      defaultInfoDiagnosticAction(DiagnosticAction::Info),
      defaultWarningDiagnosticAction(DiagnosticAction::Warn) {
    ErrorCatalog::getCatalog().initReporter(*this);
}

ErrorReporter::~ErrorReporter() = default;

ErrorReporter::ErrorReporter(const ErrorReporter &other) : ErrorReporter() { *this = other; }

ErrorReporter &ErrorReporter::operator=(const ErrorReporter &other) {
    if (this == &other) return *this;
    collectingDiagnostics = other.collectingDiagnostics;
    diagnostics = other.diagnostics;
    diagnosticConsumer = {};
    sarifWriter.reset();
    infoCount = other.infoCount;
    warningCount = other.warningCount;
    errorCount = other.errorCount;
    maxErrorCount = other.maxErrorCount;
    outputstream = other.outputstream;
    errorTracker = other.errorTracker;
    defaultInfoDiagnosticAction = other.defaultInfoDiagnosticAction;
    defaultWarningDiagnosticAction = other.defaultWarningDiagnosticAction;
    diagnosticActions = other.diagnosticActions;
    return *this;
}

bool ErrorReporter::setDiagnosticsFile(const std::string &file) {
    if (sarifWriter && sarifWriter->filename() == file) return true;
    enableDiagnosticCollection();
    auto writer = std::make_unique<SarifWriter>(file);
    if (!writer->write(diagnostics)) {
        diagnose(DiagnosticAction::Error, "io", "Cannot write diagnostics file %s", "", file);
        return false;
    }
    sarifWriter = std::move(writer);
    return true;
}

void ErrorReporter::publishDiagnostic(Diagnostic diagnostic) {
    auto index = diagnostics.size();
    diagnostics.push_back(std::move(diagnostic));
    std::optional<std::string> failedFile;
    if (sarifWriter && !sarifWriter->write(diagnostics)) {
        failedFile = sarifWriter->filename();
        // Disable the broken sink before reporting its error, avoiding recursion.
        sarifWriter.reset();
    }
    if (diagnosticConsumer) diagnosticConsumer(diagnostics[index]);
    if (failedFile)
        diagnose(DiagnosticAction::Error, "io", "Cannot write diagnostics file %s", "",
                 *failedFile);
}

void ErrorReporter::reportDiagnostic(Diagnostic diagnostic) {
    switch (diagnostic.severity) {
        case DiagnosticSeverity::Error:
            ++errorCount;
            break;
        case DiagnosticSeverity::Warning:
            ++warningCount;
            break;
        case DiagnosticSeverity::Note:
            ++infoCount;
            break;
    }
    if (diagnostic.location) {
        *outputstream << diagnostic.location->file;
        if (diagnostic.location->range) {
            const auto &start = diagnostic.location->range->start;
            *outputstream << ':' << start.line + 1;
            if (start.column) *outputstream << ':' << *start.column + 1;
        }
        *outputstream << ": ";
    }
    *outputstream << diagnosticSeverityName(diagnostic.severity) << ": " << diagnostic.message;
    if (!diagnostic.code.empty()) *outputstream << " [" << diagnostic.code << ']';
    *outputstream << '\n';
    outputstream->flush();
    if (collectingDiagnostics) publishDiagnostic(std::move(diagnostic));
}

}  // namespace P4
