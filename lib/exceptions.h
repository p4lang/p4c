/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -*-c++-*- */

#ifndef LIB_EXCEPTIONS_H_
#define LIB_EXCEPTIONS_H_

#include <unistd.h>

#include <exception>
#include <source_location>
#include <string>
#include <utility>

#include "lib/format.h"

namespace P4::Util {

// colors to pretty print messages
constexpr char ANSI_RED[] = "\33[31m";
constexpr char ANSI_BLUE[] = "\33[34m";
constexpr char ANSI_CLR[] = "\33[0m";

/// Checks if stderr is redirected to a file
/// Check is done only once and then saved to a static variable
inline bool is_cerr_redirected() {
    static bool initialized(false);
    static bool is_redir;
    if (!initialized) {
        initialized = true;
        is_redir = ttyname(fileno(stderr)) == nullptr;  // NOLINT(runtime/threadsafe_fn)
    }
    return is_redir;
}

/// Function used to delete color in case of cerr output being redirected
inline const char *cerr_colorize(const char *color) {
    if (is_cerr_redirected()) {
        return "";
    }
    return color;
}

/// Used to clear colors on terminal
inline const char *cerr_clear_colors() {
    if (is_cerr_redirected()) {
        return "";
    }
    return ANSI_CLR;
}

/// Base class for all exceptions.
/// Supports boost-style (%N%) and printf/absl::StrFormat-style (%s, %d, etc.)
/// placeholders.
class P4CExceptionBase : public std::exception {
 protected:
    std::string message;
    void traceCreation() {}

    /// Add an exception heading to already-formatted text without invoking the formatter.
    P4CExceptionBase(std::string formattedMessage, const char *heading, const char *color);
    P4CExceptionBase(std::source_location location, std::string formattedMessage,
                     const char *heading, const char *color);

 public:
    /// Use already-formatted text without invoking the diagnostic formatter.
    explicit P4CExceptionBase(std::string formattedMessage) : message(std::move(formattedMessage)) {
        traceCreation();
    }

    template <typename... Args>
    explicit P4CExceptionBase(const char *format, Args &&...args) {
        traceCreation();
        message = createBugMessage(format, args...);
    }

    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/// This class indicates a bug in the compiler
class CompilerBug final : public P4CExceptionBase {
 public:
    /// Also usable when the diagnostic formatter itself fails.
    explicit CompilerBug(std::string formattedMessage)
        : P4CExceptionBase(std::move(formattedMessage), "Compiler Bug", ANSI_RED) {}

    template <typename... Args>
    explicit CompilerBug(const char *format, Args &&...args)
        : CompilerBug(createBugMessage(format, args...)) {}

    template <typename... Args>
    CompilerBug(std::source_location location, const char *format, Args &&...args)
        : P4CExceptionBase(location, createBugMessage(format, args...), "Compiler Bug", ANSI_RED) {}
};

/// This class indicates an unimplemented feature in the compiler
class CompilerUnimplemented final : public P4CExceptionBase {
 public:
    template <typename... Args>
    explicit CompilerUnimplemented(const char *format, Args &&...args)
        : P4CExceptionBase(createBugMessage(format, args...), "Not yet implemented", ANSI_BLUE) {}

    template <typename... Args>
    CompilerUnimplemented(std::source_location location, const char *format, Args &&...args)
        : P4CExceptionBase(location, createBugMessage(format, args...),
                           "Unimplemented compiler support", ANSI_BLUE) {}
};

/// This class indicates a compilation error that we do not want to recover from.
/// This may be due to a malformed input program.
class CompilationError : public P4CExceptionBase {
 public:
    template <typename... Args>
    explicit CompilationError(const char *format, Args &&...args)
        : P4CExceptionBase(format, std::forward<Args>(args)...) {}
};

/// Capture the caller's location when a format string is passed to a variadic function.
/// A defaulted source_location parameter after an argument pack would prevent deduction.
struct FormatWithLocation {
    const char *format;
    std::source_location location;

    // The implicit conversion captures the location at the call site.
    // NOLINTNEXTLINE(runtime/explicit)
    constexpr FormatWithLocation(const char *format,
                                 std::source_location location = std::source_location::current())
        : format(format), location(location) {}
};

}  // namespace P4::Util

template <typename... Args>
[[noreturn]] inline void BUG(P4::Util::FormatWithLocation fmt, Args &&...args) {
    throw P4::Util::CompilerBug(fmt.location, fmt.format, std::forward<Args>(args)...);
}

// Keep diagnostic arguments unevaluated when the check succeeds.
#define BUG_CHECK(e, ...)             \
    do {                              \
        if (!(e)) ::BUG(__VA_ARGS__); \
    } while (0)

template <typename... Args>
[[noreturn]] inline void P4C_UNIMPLEMENTED(P4::Util::FormatWithLocation fmt, Args &&...args) {
    throw P4::Util::CompilerUnimplemented(fmt.location, fmt.format, std::forward<Args>(args)...);
}

/// Report an error and exit
template <typename... Args>
[[noreturn]] inline void FATAL_ERROR(const char *format, Args &&...args) {
    throw P4::Util::CompilationError(format, std::forward<Args>(args)...);
}

#endif /* LIB_EXCEPTIONS_H_ */
