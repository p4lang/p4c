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

#include <stdarg.h>
#include <boost/format.hpp>
#include <type_traits>

#include "lib/cstring.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

// All these methods return std::string because this is the native format of boost::format
// If tail is not empty, it is appended on a new line
// Position is printed at the beginning.
static inline std::string error_helper(boost::format& f, std::string message,
                                       std::string position, std::string tail) {
    std::string text = boost::str(f);
    std::string result = position;
    if (!position.empty())
        result += ": ";
    result += message + text + "\n" + tail;
    return result;
}

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const char* t, Args... args);

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const cstring& t, Args... args);

template<class... Args>
std::string error_helper(boost::format& f, std::string message,
                         std::string position, std::string tail,
                         const Util::SourceInfo &info, Args... args);

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T &t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T *t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const mpz_class *t, Args... args);

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const mpz_class &t, Args... args);

template<typename T, class... Args>
auto
error_helper(boost::format& f, std::string message,
             std::string position, std::string tail,
             const T& t, Args... args) ->
    typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type;

// actual implementations

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const char* t, Args... args) {
    return error_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const cstring& t, Args... args) {
    return error_helper(f % t.c_str(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T &t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    return error_helper(f % t.toString(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T *t, Args... args) ->
    typename std::enable_if<Util::HasToString<T>::value &&
                            !std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    return error_helper(f % t->toString(), message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const mpz_class *t, Args... args) {
    return error_helper(f % t->get_str(), message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const mpz_class &t, Args... args) {
    return error_helper(f % t.get_str(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto
error_helper(boost::format& f, std::string message, std::string position,
             std::string tail, const T& t, Args... args) ->
    typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type {
    return error_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string error_helper(boost::format& f, std::string message, std::string position,
                         std::string tail, const Util::SourceInfo &info, Args... args) {
    cstring posString = info.toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    return error_helper(f % "", message, position, tail + posString + info.toSourceFragment(),
                        std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    cstring posString = t->getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    return error_helper(f % t->toString(), message, position,
                        tail + posString + t->getSourceInfo().toSourceFragment(),
                        std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto error_helper(boost::format& f, std::string message, std::string position,
                  std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    cstring posString = t.getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    return error_helper(f % t.toString(), message, position,
                        tail + posString + t.getSourceInfo().toSourceFragment(),
                        std::forward<Args>(args)...);
}

/***********************************************************************************/

static inline std::string bug_helper(boost::format& f, std::string message,
                                     std::string position, std::string tail) {
    std::string text = boost::str(f);
    std::string result = position;
    if (!position.empty())
        result += ": ";
    result += message + text + "\n" + tail;
    return result;
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const char* t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const cstring& t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message,
                       std::string position, std::string tail,
                       const Util::SourceInfo &info, Args... args);

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type;

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const mpz_class *t, Args... args);

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const mpz_class &t, Args... args);

template<typename T, class... Args>
auto
bug_helper(boost::format& f, std::string message, std::string position,
           std::string tail, const T& t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                            std::string>::type;

// actual implementations

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const char* t, Args... args) {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const cstring& t, Args... args) {
    return bug_helper(f % t.c_str(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
        typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                                std::string>::type {
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const mpz_class *t, Args... args) {
    return bug_helper(f % t->get_str(), message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const mpz_class &t, Args... args) {
    return bug_helper(f % t.get_str(), message, position, tail, std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto
bug_helper(boost::format& f, std::string message, std::string position,
           std::string tail, const T& t, Args... args) ->
    typename std::enable_if<!std::is_base_of<Util::IHasSourceInfo, T>::value,
                            std::string>::type {
    return bug_helper(f % t, message, position, tail, std::forward<Args>(args)...);
}

template<class... Args>
std::string bug_helper(boost::format& f, std::string message, std::string position,
                       std::string tail, const Util::SourceInfo &info, Args... args) {
    cstring posString = info.toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    return bug_helper(f % "", message, position, tail + posString + info.toSourceFragment(),
                        std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T *t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    if (t == nullptr) {
        return bug_helper(f, message, position,
                          tail, std::forward<Args>(args)...);
    }

    cstring posString = t->getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position,
                      tail + posString + t->getSourceInfo().toSourceFragment(),
                      std::forward<Args>(args)...);
}

template<typename T, class... Args>
auto bug_helper(boost::format& f, std::string message, std::string position,
                std::string tail, const T &t, Args... args) ->
    typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value, std::string>::type {
    cstring posString = t.getSourceInfo().toPositionString();
    if (position.empty()) {
        position = posString;
        posString = "";
    } else {
        if (!posString.isNullOrEmpty())
            posString += "\n";
    }
    std::stringstream str;
    str << t;
    return bug_helper(f % str.str(), message, position,
                        tail + posString + t.getSourceInfo().toSourceFragment(),
                        std::forward<Args>(args)...);
}

/***********************************************************************************/

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

    void emit_message(cstring message) {
        *outputstream << message;
        outputstream->flush();
    }

 public:
    ErrorReporter()
        : errorCount(0),
          warningCount(0)
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

    template <typename... T>
    void diagnose(DiagnosticAction action, const char* diagnosticName,
                  const char* format, T... args) {
        if (action == DiagnosticAction::Ignore) return;

        std::string prefix;
        if (action == DiagnosticAction::Warn) {
            warningCount++;
            prefix.append("[--Wwarn=");
            prefix.append(diagnosticName);
            prefix.append("] warning: ");
        } else if (action == DiagnosticAction::Error) {
            errorCount++;
            prefix.append("[--Werror=");
            prefix.append(diagnosticName);
            prefix.append("] error: ");
        }

        boost::format fmt(format);
        std::string message = ::error_helper(fmt, prefix, "", "", args...);
        emit_message(message);
    }

    template <typename... T>
    void diagnoseUnnamed(DiagnosticAction action, const char* format, T... args) {
        const char* msg;
        if (action == DiagnosticAction::Error) {
            msg = "error: ";
            errorCount++;
        } else if (action == DiagnosticAction::Warn) {
            msg = "warning: ";
            warningCount++;
        } else {
            return;
        }
        boost::format fmt(format);
        std::string message = ::error_helper(fmt, msg, "", "", args...);
        emit_message(message);
    }

    unsigned getErrorCount() const {
        return errorCount;
    }

    unsigned getWarningCount() const {
        return warningCount;
    }

    /// @return the number of diagnostics (warnings and errors) encountered
    /// in the current CompileContext.
    unsigned getDiagnosticCount() const {
        return errorCount + warningCount;
    }

    void setOutputStream(std::ostream* stream)
    { outputstream = stream; }

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

 private:
    unsigned errorCount;
    unsigned warningCount;
};

#endif /* P4C_LIB_ERROR_REPORTER_H_ */
