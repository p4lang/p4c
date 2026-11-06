// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#ifndef FRONTENDS_COMMON_PREPROCESSORDIAGNOSTICS_H_
#define FRONTENDS_COMMON_PREPROCESSORDIAGNOSTICS_H_

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

namespace P4 {
class ErrorReporter;

/// Decode GCC JSON into the canonical model. Malformed/unsupported records are
/// reported as errors, retaining diagnostics decoded before the failing record.
bool reportPreprocessorDiagnostics(std::string_view json, ErrorReporter &reporter);

/// Run the existing preprocessor command with separate temporary stdout/stderr
/// files. Waiting cannot block on pipe capacity. Return rewound stdout on success.
using PreprocessorOutput = std::unique_ptr<FILE, int (*)(FILE *)>;
PreprocessorOutput preprocessWithDiagnostics(const std::string &command, ErrorReporter &reporter);

}  // namespace P4

#endif /* FRONTENDS_COMMON_PREPROCESSORDIAGNOSTICS_H_ */
