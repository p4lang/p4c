/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_ERROR_REPORTER_H_
#define LIB_ERROR_REPORTER_H_

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "error_catalog.h"
#include "error_helper.h"
#include "exceptions.h"
#include "lib/diagnostic.h"
#include "lib/format.h"
#include "lib/source_file.h"

namespace P4 {

class SarifWriter;

/// An action to take when a diagnostic message is triggered.
enum class DiagnosticAction {
    Ignore,  /// Take no action and continue compilation.
    Info,    /// Print an info message and continue compilation.
    Warn,    /// Print a warning and continue compilation.
    Error    /// Print an error and signal that compilation should be aborted.
};

// Keeps track of compilation errors.
// Errors are specified using the error() and warning() methods.
// Supported formatting styles are boost-style (%1%, %2%, ...) and
// printf/absl::StrFormat-style (%s, %d, ...).
class ErrorReporter {
    bool collectingDiagnostics = false;
    std::vector<Diagnostic> diagnostics;
    std::function<void(const Diagnostic &)> diagnosticConsumer;
    std::unique_ptr<SarifWriter> sarifWriter;

    void publishDiagnostic(Diagnostic diagnostic);

 protected:
    unsigned int infoCount;
    unsigned int warningCount;
    unsigned int errorCount;
    unsigned int maxErrorCount;  /// the maximum number of errors that we print before fail

    std::ostream *outputstream;

    /// Track errors or warnings that have already been issued for a particular source location
    std::set<std::pair<int, const Util::SourceInfo>> errorTracker;

    /// Output the message and flush the stream
    virtual void emit_message(const ErrorMessage &msg) {
        *outputstream << msg.toString();
        outputstream->flush();
    }

    virtual void emit_message(const ParserErrorMessage &msg) {
        *outputstream << msg.toString();
        outputstream->flush();
    }

    /// Check whether an error has already been reported, by keeping track of error type
    /// and source info.
    /// If the error has been reported, return true. Otherwise, insert add the error to the
    /// list of seen errors, and return false.
    bool error_reported(int err, const Util::SourceInfo source) {
        if (!source.isValid()) return false;
        auto p = errorTracker.emplace(err, source);
        return !p.second;  // if insertion took place, then we have not seen the error.
    }

    /// retrieve the format from the error catalog
    cstring get_error_name(int errorCode) { return ErrorCatalog::getCatalog().getName(errorCode); }

 public:
    ErrorReporter();
    /// Copy diagnostic state and policy, without sharing consumers or file sinks.
    ErrorReporter(const ErrorReporter &other);
    ErrorReporter &operator=(const ErrorReporter &other);
    virtual ~ErrorReporter();

    /// Enable owned, compilation-scoped snapshots. Terminal output is unchanged.
    void enableDiagnosticCollection() { collectingDiagnostics = true; }
    bool isCollectingDiagnostics() const { return collectingDiagnostics; }

    /// Return a copy that remains valid after this reporter/context is destroyed.
    std::vector<Diagnostic> getDiagnostics() const { return diagnostics; }

    /// Optional synchronous observer, invoked after storing and writing a record.
    /// Copy records to retain them; the callback must not reenter this reporter.
    void setDiagnosticConsumer(std::function<void(const Diagnostic &)> consumer) {
        enableDiagnosticCollection();
        diagnosticConsumer = std::move(consumer);
    }

    /// Initialize an optional SARIF adapter; failures are compiler I/O errors.
    bool setDiagnosticsFile(const std::string &file);

    /// Import a diagnostic from an external producer (e.g. GCC). It has already
    /// passed that producer's warning policy. Print/count it without filtering it
    /// through p4c's catalog, then publish the same owned record to API consumers.
    void reportDiagnostic(Diagnostic diagnostic);

    // Error message for a bug.
    template <typename... Args>
    std::string bug_message(const char *format, Args &&...args) {
        return createBugMessage(format, args...);
    }

    // Formats a message with a trailing newline and any source locations and excerpts.
    template <typename... Args>
    std::string format_message(const char *format, Args &&...args) {
        return ::P4::error_helper(format, std::forward<Args>(args)...).toString();
    }

    template <class T, typename = decltype(std::declval<T>()->getSourceInfo()), typename... Args>
    void diagnose(DiagnosticAction action, const int errorCode, const char *format,
                  const char *suffix, T node, Args &&...args) {
        if (!node || error_reported(errorCode, node->getSourceInfo())) return;

        if (cstring name = get_error_name(errorCode))
            diagnose(getDiagnosticAction(errorCode, name, action), name.c_str(), format, suffix,
                     node, std::forward<Args>(args)...);
        else
            diagnose(action, nullptr, format, suffix, node, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void diagnose(DiagnosticAction action, const int errorCode, const char *format,
                  const char *suffix, Args &&...args) {
        if (cstring name = get_error_name(errorCode))
            diagnose(getDiagnosticAction(errorCode, name, action), name.c_str(), format, suffix,
                     std::forward<Args>(args)...);
        else
            diagnose(action, nullptr, format, suffix, std::forward<Args>(args)...);
    }

    /// The sink of all the diagnostic functions. Here the error gets printed
    /// or an exception thrown if the error count exceeds maxErrorCount.
    template <typename... Args>
    void diagnose(DiagnosticAction action, const char *diagnosticName, const char *format,
                  const char *suffix, Args &&...args) {
        if (action == DiagnosticAction::Ignore) return;

        ErrorMessage::MessageType msgType = ErrorMessage::MessageType::None;
        if (action == DiagnosticAction::Info) {
            // Avoid burying errors in a pile of info messages:
            // don't emit any more info messages if we've emitted errors.
            if (errorCount > 0) return;

            infoCount++;
            msgType = ErrorMessage::MessageType::Info;
        } else if (action == DiagnosticAction::Warn) {
            // Avoid burying errors in a pile of warnings: don't emit any more warnings if we've
            // emitted errors.
            if (errorCount > 0) return;

            warningCount++;
            msgType = ErrorMessage::MessageType::Warning;
        } else if (action == DiagnosticAction::Error) {
            errorCount++;
            msgType = ErrorMessage::MessageType::Error;
        }

        ErrorMessage msg(msgType, diagnosticName ? diagnosticName : "", suffix);
        msg = ::P4::error_helper(format, std::move(msg), std::forward<Args>(args)...);
        if (collectingDiagnostics) publishDiagnostic(makeDiagnostic(msg));
        emit_message(msg);

        if (errorCount >= maxErrorCount)
            FATAL_ERROR("Number of errors exceeded set maximum of %1%", maxErrorCount);
    }

    unsigned getErrorCount() const { return errorCount; }

    unsigned getMaxErrorCount() const { return maxErrorCount; }
    /// set maxErrorCount to a the @newMaxCount threshold and return the previous value
    unsigned setMaxErrorCount(unsigned newMaxCount) {
        auto r = maxErrorCount;
        maxErrorCount = newMaxCount;
        return r;
    }

    unsigned getWarningCount() const { return warningCount; }

    unsigned getInfoCount() const { return infoCount; }

    /// @return the number of diagnostics (warnings and errors) encountered
    /// in the current CompileContext.
    unsigned getDiagnosticCount() const { return errorCount + warningCount + infoCount; }

    void setOutputStream(std::ostream *stream) { outputstream = stream; }

    std::ostream *getOutputStream() const { return outputstream; }

    /// Reports an error @message at @location. This allows us to use the
    /// position information provided by Bison.
    template <typename T>
    void parser_error(const Util::SourceInfo &location, const T &message) {
        errorCount++;
        std::stringstream ss;
        ss << message;

        ParserErrorMessage diagnostic(location, ss.str());
        if (collectingDiagnostics) publishDiagnostic(makeDiagnostic(diagnostic));
        emit_message(diagnostic);
    }

    /**
     * Reports an error specified by @fmt and @args at the current position in
     * the input represented by @sources. This is necessary for the IR
     * generator's C-based Bison parser, which doesn't have location information
     * available.
     */
    template <typename... Args>
    void parser_error(const Util::InputSources *sources, const char *fmt, Args &&...args) {
        errorCount++;

        Util::SourcePosition position = sources->getCurrentPosition();
        position--;

        // Unfortunately, we cannot go with statically checked format string
        // here as it would require some changes to yyerror
        std::string message;
        if (!absl::FormatUntyped(&message, absl::UntypedFormatSpec(fmt),
                                 {absl::FormatArg(args)...})) {
            BUG("Failed to format string %s", fmt);
        }

        ParserErrorMessage diagnostic(Util::SourceInfo(sources, position), std::move(message));
        if (collectingDiagnostics) publishDiagnostic(makeDiagnostic(diagnostic));
        emit_message(diagnostic);
    }

    /// @return the action to take for the given diagnostic, falling back to the
    /// default action if it wasn't overridden via the command line or a pragma.
    DiagnosticAction getDiagnosticAction(int errorCode, cstring diagnostic,
                                         DiagnosticAction defaultAction) {
        // Actions for errors can never be overridden.
        if (ErrorCatalog::getCatalog().isError(errorCode)) return DiagnosticAction::Error;
        auto it = diagnosticActions.find(diagnostic);
        if (it != diagnosticActions.end()) return it->second;
        // if we're dealing with warnings and they have been globally modified
        // (ignored or turned into errors), then return the global default
        if (defaultAction == DiagnosticAction::Warn &&
            defaultWarningDiagnosticAction != DiagnosticAction::Warn)
            return defaultWarningDiagnosticAction;
        return defaultAction;
    }

    /// Set the action to take for the given diagnostic.
    void setDiagnosticAction(std::string_view diagnostic, DiagnosticAction action) {
        diagnosticActions[cstring(diagnostic)] = action;
    }

    /// @return the default diagnostic action for calls to `::P4::warning()`.
    DiagnosticAction getDefaultWarningDiagnosticAction() { return defaultWarningDiagnosticAction; }

    /// set the default diagnostic action for calls to `::P4::warning()`.
    void setDefaultWarningDiagnosticAction(DiagnosticAction action) {
        defaultWarningDiagnosticAction = action;
    }

    /// @return the default diagnostic action for calls to `::P4::info()`.
    DiagnosticAction getDefaultInfoDiagnosticAction() { return defaultInfoDiagnosticAction; }

    /// set the default diagnostic action for calls to `::P4::info()`.
    void setDefaultInfoDiagnosticAction(DiagnosticAction action) {
        defaultInfoDiagnosticAction = action;
    }

 private:
    /// The default diagnostic action for calls to `::P4::info()`.
    DiagnosticAction defaultInfoDiagnosticAction;

    /// The default diagnostic action for calls to `::P4::warning()`.
    DiagnosticAction defaultWarningDiagnosticAction;

    /// allow filtering of diagnostic actions
    std::unordered_map<cstring, DiagnosticAction> diagnosticActions;
};

}  // namespace P4

#endif /* LIB_ERROR_REPORTER_H_ */
