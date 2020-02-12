/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef P4C_LIB_ERROR_REPORTER_H_
#define P4C_LIB_ERROR_REPORTER_H_

#include "error_helper.h"
#include "error_catalog.h"
#include "exceptions.h"

/// An action to take when a diagnostic message is triggered.
enum class DiagnosticAction {
    Ignore,  /// Take no action and continue compilation.
    Warn,    /// Print a warning and continue compilation.
    Error    /// Print an error and signal that compilation should be aborted.
};


// Keeps track of compilation errors.
// Errors are specified using the error() and warning() methods,
// that use boost::format format strings, i.e.,
// %1%, %2%, etc (starting at 1, not at 0).
// Some compatibility for printf-style arguments is also supported.
class ErrorReporter final {
 private:
    std::ostream* outputstream;

    /// Track errors or warnings that have already been issued for a particular source location
    std::set<std::pair<int, const Util::SourceInfo>> errorTracker;

    /// Output the message and flush the stream
    void emit_message(cstring message) {
        *outputstream << message;
        outputstream->flush();
    }

    /// Check whether an error has already been reported, by keeping track of error type
    /// and source info.
    /// If the error has been reported, return true. Otherwise, insert add the error to the
    /// list of seen errors, and return false.
    bool error_reported(int err, const Util::SourceInfo source) {
        auto p = errorTracker.emplace(err, source);
        return !p.second;  // if insertion took place, then we have not seen the error.
    }

    /// retrieve the format from the error catalog
    const char *get_error_name(int errorCode) {
        return ErrorCatalog::getCatalog().getName(errorCode);
    }

 public:
    ErrorReporter()
        : errorCount(0),
          warningCount(0),
          maxErrorCount(20),
          defaultWarningDiagnosticAction(DiagnosticAction::Warn)
    { outputstream = &std::cerr; }

    // error message for a bug
    template <typename... T>
    std::string bug_message(const char* format, T... args) {
        boost::format fmt(format);
        std::string message = ::bug_helper(fmt, "", "", "", args...);
        return message;
    }

    template <typename... T>
    std::string format_message(const char* format, T... args) {
        boost::format fmt(format);
        std::string message = ::error_helper(fmt, "", "", "", args...);
        return message;
    }

    template <class T,
              typename = typename std::enable_if<std::is_base_of<Util::IHasSourceInfo,
                                                                 T>::value>::type,
              typename... Args>
    void diagnose(DiagnosticAction action, const int errorCode, const char *format, const T *node,
                  Args... args) {
        if (!error_reported(errorCode, node->getSourceInfo())) {
            const char *name = get_error_name(errorCode);
            auto da = getDiagnosticAction(name, action);
            if (name)
                diagnose(da, name, format, node, args...);
            else
                diagnose(action, nullptr, format, node, std::forward<Args>(args)...);
        }
    }

    template <class T,
              typename = typename std::enable_if<std::is_base_of<Util::IHasSourceInfo,
                                                                 T>::value>::type,
              typename... Args>
    void diagnose(DiagnosticAction action, const int errorCode, const char *format, const T &node,
                  Args... args) {
        diagnose(action, errorCode, format, &node, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void diagnose(DiagnosticAction action, const int errorCode, const char *format, Args... args) {
        const char *name = get_error_name(errorCode);
        auto da = getDiagnosticAction(name, action);
        if (name)
            diagnose(da, name, format, args...);
        else
            diagnose(action, nullptr, format, std::forward<Args>(args)...);
    }

    /// The sink of all the diagnostic functions. Here the error gets printed
    /// or an exception thrown if the error count exceeds maxErrorCount.
    template <typename... T>
    void diagnose(DiagnosticAction action, const char* diagnosticName,
                  const char* format, T... args) {
        if (action == DiagnosticAction::Ignore) return;

        std::string prefix;
        if (action == DiagnosticAction::Warn) {
            // Avoid burying errors in a pile of warnings: don't emit any more warnings if we've
            // emitted errors.
            if (errorCount > 0) return;

            warningCount++;
            if (diagnosticName != nullptr) {
                prefix.append("[--Wwarn=");
                prefix.append(diagnosticName);
                prefix.append("] warning: ");
            } else {
                prefix.append("warning: ");
            }
        } else if (action == DiagnosticAction::Error) {
            errorCount++;
            if (diagnosticName != nullptr) {
                prefix.append("[--Werror=");
                prefix.append(diagnosticName);
                prefix.append("] error: ");
            } else {
                prefix.append("error: ");
            }
        }

        boost::format fmt(format);
        std::string message = ::error_helper(fmt, prefix, "", "", args...);
        emit_message(message);
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

    /// @return the number of diagnostics (warnings and errors) encountered
    /// in the current CompileContext.
    unsigned getDiagnosticCount() const { return errorCount + warningCount; }

    void setOutputStream(std::ostream* stream) { outputstream = stream; }

    std::ostream* getOutputStream() const { return outputstream; }

    /// Reports an error @message at @location. This allows us to use the
    /// position information provided by Bison.
    template <typename T>
    void parser_error(const Util::SourceInfo& location, const T& message) {
        errorCount++;
        *outputstream << location.toPositionString() << ":" << message << std::endl;
        emit_message(location.toSourceFragment());  // This flushes the stream.
    }

    /**
     * Reports an error specified by @fmt and @args at the current position in
     * the input represented by @sources. This is necessary for the IR
     * generator's C-based Bison parser, which doesn't have location information
     * available.
     */
    void parser_error(const Util::InputSources* sources, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);

        errorCount++;

        Util::SourcePosition position = sources->getCurrentPosition();
        position--;
        Util::SourceFileLine fileError =
                sources->getSourceLine(position.getLineNumber());
        cstring msg = Util::vprintf_format(fmt, args);
        *outputstream << fileError.toString() << ":" << msg << std::endl;
        cstring sourceFragment = sources->getSourceFragment(position);
        emit_message(sourceFragment);

        va_end(args);
    }

    /// @return the action to take for the given diagnostic, falling back to the
    /// default action if it wasn't overridden via the command line or a pragma.
    DiagnosticAction
    getDiagnosticAction(cstring diagnostic, DiagnosticAction defaultAction) {
        auto it = diagnosticActions.find(diagnostic);
        if (it != diagnosticActions.end()) return it->second;
        // if we're dealing with warnings and they have been globally modified
        // (ingnored or turned into errors), then return the global default
        if (defaultAction == DiagnosticAction::Warn &&
            defaultWarningDiagnosticAction != DiagnosticAction::Warn)
            return defaultWarningDiagnosticAction;
        return defaultAction;
    }

    /// Set the action to take for the given diagnostic.
    void setDiagnosticAction(cstring diagnostic, DiagnosticAction action) {
        diagnosticActions[diagnostic] = action;
    }

    /// @return the default diagnostic action for calls to `::warning()`.
    DiagnosticAction getDefaultWarningDiagnosticAction() {
        return defaultWarningDiagnosticAction;
    }

    /// set the default diagnostic action for calls to `::warning()`.
    void setDefaultWarningDiagnosticAction(DiagnosticAction action) {
        defaultWarningDiagnosticAction = action;
    }

 private:
    unsigned errorCount;
    unsigned warningCount;
    unsigned maxErrorCount;  /// the maximum number of errors that we print before fail

    /// The default diagnostic action for calls to `::warning()`.
    DiagnosticAction defaultWarningDiagnosticAction;

    /// allow filtering of diagnostic actions
    std::unordered_map<cstring, DiagnosticAction> diagnosticActions;
};

#endif /* P4C_LIB_ERROR_REPORTER_H_ */
