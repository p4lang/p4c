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

/* -*-C++-*- */

#ifndef P4C_LIB_ERROR_H_
#define P4C_LIB_ERROR_H_

#include <stdarg.h>
#include <boost/format.hpp>
#include <type_traits>

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
bug_helper(boost::format& f, std::string message,
           std::string position, std::string tail,
           const T& t, Args... args) ->
    typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type;

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
    typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type {
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

// Keeps track of compilation errors.
// Singleton pattern.
// Errors are specified using the error() and warning() methods,
// that use boost::format format strings, i.e.,
// %1%, %2%, etc (starting at 1, not at 0).
// Some compatibility for printf-style arguments is also supported.
class ErrorReporter final {
 public:
    static ErrorReporter instance;

 private:
    std::ostream* outputstream;
    bool          warningsAreErrors = false;

    ErrorReporter()
        : errorCount(0),
          warningCount(0)
    { outputstream = &std::cerr; }

 private:
    void emit_message(cstring message) {
        *outputstream << message;
        outputstream->flush();
    }

 public:
    // error message for a bug
    template <typename... T>
    std::string bug_message(const char* format, T... args) {
        boost::format fmt(format);
        std::string message = ::bug_helper(fmt, "", "", "", args...);
        return message;
    }

    void setWarningsAreErrors()
    { warningsAreErrors = true; }

    template <typename... T>
    std::string format_message(const char* format, T... args) {
        boost::format fmt(format);
        std::string message = ::error_helper(fmt, "", "", "", args...);
        return message;
    }

    template <typename... T>
    void error(const char* format, T... args) {
        errorCount++;
        boost::format fmt(format);
        std::string message = ::error_helper(fmt, "error: ", "", "", args...);
        emit_message(message);
    }

    template <typename... T>
    void warning(const char* format, T... args) {
        const char* msg;
        if (warningsAreErrors) {
            msg = "error: ";
            errorCount++;
        } else {
            msg = "warning: ";
            warningCount++;
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

    // Resets this ErrorReporter to its initial state.
    void clear() {
      errorCount = 0;
      warningCount = 0;
    }

    // Special error functions to be called from the parser only.
    // In the parser the IR objects don't yet have position information.
    // Use printf-format style arguments.
    // Use current position in the input file for error location.
    void parser_error(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        parser_error(fmt, args);
        va_end(args);
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
     * the input file. This is necessary for the IR generator's C-based Bison
     * parser, which doesn't have location information available.
     */
    void parser_error(const char* fmt, va_list args) {
        errorCount++;

        Util::SourcePosition position = Util::InputSources::instance->getCurrentPosition();
        position--;
        Util::SourceFileLine fileError =
                Util::InputSources::instance->getSourceLine(position.getLineNumber());
        cstring msg = Util::vprintf_format(fmt, args);
        *outputstream << fileError.toString() << ":" << msg << std::endl;
        cstring sourceFragment = Util::InputSources::instance->getSourceFragment(position);
        emit_message(sourceFragment);
    }

 private:
    unsigned errorCount;
    unsigned warningCount;
};

// Errors (and warnings) are specified using boost::format format strings, i.e.,
// %1%, %2%, etc (starting at 1, not at 0).
// Some compatibility for printf-style arguments is also supported.
template <typename... T>
inline void error(const char* format, T... args) {
    ErrorReporter::instance.error(format, args...);
}

inline unsigned errorCount() { return ErrorReporter::instance.getErrorCount(); }

template <typename... T>
inline void warning(const char* format, T... args) {
    ErrorReporter::instance.warning(format, args...);
}

inline void clearErrorReporter() {
  ErrorReporter::instance.clear();
}

#endif /* P4C_LIB_ERROR_H_ */
