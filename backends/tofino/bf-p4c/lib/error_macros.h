/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_LIB_ERROR_MACROS_H_
#define BACKENDS_TOFINO_BF_P4C_LIB_ERROR_MACROS_H_

/// Report an error if condition e is false.
#define ERROR_CHECK(e, ...)                 \
    do {                                    \
        if (!(e)) ::P4::error(__VA_ARGS__); \
    } while (0)

/// Report a warning if condition e is false.
#define WARN_CHECK(e, ...)                    \
    do {                                      \
        if (!(e)) ::P4::warning(__VA_ARGS__); \
    } while (0)

/// Trigger a diagnostic message which is treated as a warning by default.
#define DIAGNOSE_WARN(DIAGNOSTIC_NAME, ...)                               \
    do {                                                                  \
        ::diagnose(DiagnosticAction::Warn, DIAGNOSTIC_NAME, __VA_ARGS__); \
    } while (0)

/// Trigger a diagnostic message which is treated as an error by default.
#define DIAGNOSE_ERROR(DIAGNOSTIC_NAME, ...)                               \
    do {                                                                   \
        ::diagnose(DiagnosticAction::Error, DIAGNOSTIC_NAME, __VA_ARGS__); \
    } while (0)

#endif /* BACKENDS_TOFINO_BF_P4C_LIB_ERROR_MACROS_H_ */
