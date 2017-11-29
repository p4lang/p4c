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

#include "lib/error_reporter.h"

/// @return the number of errors encountered so far in the current compilation
/// context.
inline unsigned errorCount() {
    return BaseCompileContext::get().errorReporter().getErrorCount();
}

/// @return the number of diagnostics (either errors or warnings) encountered so
/// far in the current compilation context.
inline unsigned diagnosticCount() {
    return BaseCompileContext::get().errorReporter().getDiagnosticCount();
}

// Errors (and warnings) are specified using boost::format format strings, i.e.,
// %1%, %2%, etc (starting at 1, not at 0).
// Some compatibility for printf-style arguments is also supported.

/// Report an error with the given message.
template <typename... T>
inline void error(const char* format, T... args) {
    BaseCompileContext::get().errorReporter().error(format, args...);
}

#define ERROR_CHECK(e, ...) do { if (!(e)) ::error(__VA_ARGS__); } while (0)

/// Report a warning with the given message.
template <typename... T>
inline void warning(const char* format, T... args) {
    BaseCompileContext::get().errorReporter().warning(format, args...);
}

#define WARN_CHECK(e, ...) do { if (!(e)) ::warning(__VA_ARGS__); } while (0)

#endif /* P4C_LIB_ERROR_H_ */
