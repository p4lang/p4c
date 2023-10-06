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

#ifndef LIB_LOG_H_
#define LIB_LOG_H_

#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "config.h"
#include "indent.h"

#ifndef __GNUC__
#define __attribute__(X)
#endif

namespace Log {
namespace Detail {
// The global verbosity level.
extern int verbosity;

// A cache of the maximum log level requested for any file.
extern int maximumLogLevel;

// Used to restrict logging to a specific IR context.
extern bool enableLoggingGlobally;
extern bool enableLoggingInContext;  // if enableLoggingGlobally is true, this is ignored.

// Look up the log level of @file.
int fileLogLevel(const char *file);
std::ostream &fileLogOutput(const char *file);

// A utility class used to prepend file and log level information to logging output.
// also controls indent control and locking for multithreaded use
class OutputLogPrefix {
    const char *fn;
    int level;
    static int ostream_xalloc;
    static void setup_ostream_xalloc(std::ostream &);
    friend std::ostream &operator<<(std::ostream &, const OutputLogPrefix &);
    friend std::ostream &clearPrefix(std::ostream &out);
#ifdef MULTITHREAD
    struct lock_t;
    mutable lock_t *lock = nullptr;
#endif  // MULTITHREAD
 public:
    OutputLogPrefix(const char *f, int l) : fn(f), level(l) {}
    ~OutputLogPrefix();
    static void indent(std::ostream &out);
};

void addInvalidateCallback(void (*)(void));
std::ostream &clearPrefix(std::ostream &out);
}  // namespace Detail

inline std::ostream &endl(std::ostream &out) {
    out << std::endl;
    Detail::OutputLogPrefix::indent(out);
    return out;
}
using IndentCtl::indent;
using IndentCtl::TempIndent;
using IndentCtl::unindent;

inline bool fileLogLevelIsAtLeast(const char *file, int level) {
    // If there's no file with a log level of at least @level, we don't need to do
    // the more expensive per-file check.
    if (Detail::maximumLogLevel < level) {
        return false;
    }

    return Detail::fileLogLevel(file) >= level;
}

// Process @spec and update the log level requested for the appropriate file.
void addDebugSpec(const char *spec);

inline bool verbose() { return Detail::verbosity > 0; }
inline int verbosity() { return Detail::verbosity; }
inline bool enableLogging() {
    return Detail::enableLoggingGlobally || Detail::enableLoggingInContext;
}
void increaseVerbosity();

}  // namespace Log

#ifndef MAX_LOGGING_LEVEL
// can be set on build command line and disables higher logging levels at compile time
#define MAX_LOGGING_LEVEL 10
#endif

#define LOGGING(N)                                                            \
    ((N) <= MAX_LOGGING_LEVEL && ::Log::fileLogLevelIsAtLeast(__FILE__, N) && \
     ::Log::enableLogging())
#define LOGN(N, X)                                                        \
    (LOGGING(N) ? ::Log::Detail::fileLogOutput(__FILE__)                  \
                      << ::Log::Detail::OutputLogPrefix(__FILE__, N) << X \
                      << ::Log::Detail::clearPrefix << std::endl          \
                : std::clog)
#define LOG1(X) LOGN(1, X)
#define LOG2(X) LOGN(2, X)
#define LOG3(X) LOGN(3, X)
#define LOG4(X) LOGN(4, X)
#define LOG5(X) LOGN(5, X)
#define LOG6(X) LOGN(6, X)
#define LOG7(X) LOGN(7, X)
#define LOG8(X) LOGN(8, X)
#define LOG9(X) LOGN(9, X)

#define LOGN_UNINDENT(N) \
    (LOGGING(N) ? ::Log::Detail::fileLogOutput(__FILE__) << IndentCtl::unindent : std::clog)
#define LOG1_UNINDENT LOGN_UNINDENT(1)
#define LOG2_UNINDENT LOGN_UNINDENT(2)
#define LOG3_UNINDENT LOGN_UNINDENT(3)
#define LOG4_UNINDENT LOGN_UNINDENT(4)
#define LOG5_UNINDENT LOGN_UNINDENT(5)
#define LOG6_UNINDENT LOGN_UNINDENT(6)
#define LOG7_UNINDENT LOGN_UNINDENT(7)
#define LOG8_UNINDENT LOGN_UNINDENT(8)
#define LOG9_UNINDENT LOGN_UNINDENT(9)

#define LOG_FEATURE(TAG, N, X)                                             \
    ((N) <= MAX_LOGGING_LEVEL && ::Log::fileLogLevelIsAtLeast(TAG, N)      \
         ? ::Log::Detail::fileLogOutput(TAG)                               \
               << ::Log::Detail::OutputLogPrefix(TAG, N) << X << std::endl \
         : std::clog)

#define P4C_ERROR(X) (std::clog << "ERROR: " << X << std::endl)
#define P4C_WARNING(X) (::Log::verbose() ? std::clog << "WARNING: " << X << std::endl : std::clog)
#define ERRWARN(C, X) ((C) ? P4C_ERROR(X) : P4C_WARNING(X))

static inline std::ostream &operator<<(std::ostream &out,
                                       std::function<std::ostream &(std::ostream &)> fn) {
    return fn(out);
}

template <class T>
inline auto operator<<(std::ostream &out, const T &obj) -> decltype((void)obj.dbprint(out), out) {
    obj.dbprint(out);
    return out;
}

template <class T>
inline auto operator<<(std::ostream &out, const T *obj) -> decltype((void)obj->dbprint(out), out) {
    if (obj)
        obj->dbprint(out);
    else
        out << "<null>";
    return out;
}

/// Serializes the @p container into the stream @p out, delimining it by bracketing it with
/// @p lbrace and @p rbrace. This is mostly a helper for writing operator<< fur custom container
/// classes.
template <class Cont>
std::ostream &format_container(std::ostream &out, const Cont &container, char lbrace, char rbrace) {
    std::vector<std::string> elems;
    bool foundnl = false;
    for (auto &el : container) {
        std::stringstream tmp;
        tmp << el;
        elems.emplace_back(tmp.str());
        if (!foundnl) foundnl = elems.back().find('\n') != std::string::npos;
    }
    if (foundnl) {
        for (auto &el : elems) {
            out << Log::endl << Log::indent;
            for (auto &ch : el) {
                if (ch == '\n')
                    out << Log::endl;
                else
                    out << ch;
            }
            out << Log::unindent;
        }
    } else {
        const char *sep = " ";
        out << lbrace;
        for (auto &el : elems) {
            out << sep << el;
            sep = ", ";
        }
        out << (sep + 1) << rbrace;
    }

    return out;
}

template <class T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &vec) {
    return format_container(out, vec, '[', ']');
}

template <class T>
std::ostream &operator<<(std::ostream &out, const std::set<T> &set) {
    return format_container(out, set, '(', ')');
}

#endif /* LIB_LOG_H_ */
