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

#ifndef P4C_LIB_LOG_H_
#define P4C_LIB_LOG_H_

#include <functional>
#include <iostream>
#include <set>
#include <vector>
#include "indent.h"

#include "config.h"

#ifndef __GNUC__
#define __attribute__(X)
#endif

namespace Log {
namespace Detail {
// The global verbosity level.
extern int verbosity;

// A cache of the maximum log level requested for any file.
extern int maximumLogLevel;

// Look up the log level of @file.
int fileLogLevel(const char* file);
std::ostream &fileLogOutput(const char *file);

// A utility class used to prepend file and log level information to logging output.
// also controls indent control and locking for multithreaded use
class OutputLogPrefix {
    const char* fn;
    int level;
    static int ostream_xalloc;
    static void setup_ostream_xalloc(std::ostream &);
    friend std::ostream& operator<<(std::ostream&, const OutputLogPrefix&);
#ifdef MULTITHREAD
    struct lock_t;
    mutable lock_t *lock = nullptr;
#endif  // MULTITHREAD
 public:
    OutputLogPrefix(const char* f, int l) : fn(f), level(l) {}
    ~OutputLogPrefix();
    static void indent(std::ostream &out);
};

void addInvalidateCallback(void (*)(void));
}  // namespace Detail

inline std::ostream &endl(std::ostream &out) {
    out << std::endl;
    Detail::OutputLogPrefix::indent(out);
    return out; }

inline bool fileLogLevelIsAtLeast(const char* file, int level) {
    // If there's no file with a log level of at least @level, we don't need to do
    // the more expensive per-file check.
    if (Detail::maximumLogLevel < level) {
        return false;
    }

    return Detail::fileLogLevel(file) >= level;
}

// Process @spec and update the log level requested for the appropriate file.
void addDebugSpec(const char* spec);

inline bool verbose() { return Detail::verbosity > 0; }
inline int verbosity() { return Detail::verbosity; }
void increaseVerbosity();

}  // namespace Log

#define LOGGING(N) (::Log::fileLogLevelIsAtLeast(__FILE__, N))
#define LOGN(N, X) (LOGGING(N)                                                  \
                      ? ::Log::Detail::fileLogOutput(__FILE__)                  \
                          << ::Log::Detail::OutputLogPrefix(__FILE__, N)        \
                          << X << std::endl                                     \
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

#define LOG_FEATURE(TAG, N, X) (::Log::fileLogLevelIsAtLeast(TAG, N)            \
                      ? std::clog << ::Log::Detail::OutputLogPrefix(TAG, N)     \
                                  << X << std::endl                             \
                      : std::clog)

#define ERROR(X) (std::clog << "ERROR: " << X << std::endl)
#define WARNING(X) (::Log::verbose()                               \
                      ? std::clog << "WARNING: " << X << std::endl \
                      : std::clog)
#define ERRWARN(C, X) ((C) ? ERROR(X) : WARNING(X))

static inline std::ostream &operator<<(std::ostream &out,
                                       std::function<std::ostream &(std::ostream&)> fn) {
    return fn(out); }

template<class T> inline auto operator<<(std::ostream &out, const T &obj) ->
        decltype((void)obj.dbprint(out), out)
{ obj.dbprint(out); return out; }

template<class T> inline auto operator<<(std::ostream &out, const T *obj) ->
        decltype((void)obj->dbprint(out), out) {
    if (obj)
        obj->dbprint(out);
    else
        out << "<null>";
    return out; }

template<class T> std::ostream &operator<<(std::ostream &out, const std::vector<T> &vec) {
    const char *sep = " ";
    out << '[';
    for (auto &el : vec) {
        out << sep << el;
        sep = ", "; }
    out << (sep+1) << ']';
    return out; }

template<class T> std::ostream &operator<<(std::ostream &out, const std::set<T> &vec) {
    const char *sep = " ";
    out << '(';
    for (auto &el : vec) {
        out << sep << el;
        sep = ", "; }
    out << (sep+1) << ')';
    return out; }

#endif /* P4C_LIB_LOG_H_ */
