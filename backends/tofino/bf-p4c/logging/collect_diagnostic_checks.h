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

#ifndef BACKENDS_TOFINO_BF_P4C_LOGGING_COLLECT_DIAGNOSTIC_CHECKS_H_
#define BACKENDS_TOFINO_BF_P4C_LOGGING_COLLECT_DIAGNOSTIC_CHECKS_H_
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/logging/bf_error_reporter.h"

namespace BFN {
/**
 * @brief Collects error/warning checks from the program.
 *
 * Checks are added by comments containing expect error@OFFSET: "REGEXP" and expect
 * warning@OFFSET: "REGEXP".
 *
 * OFFSET is the difference between the source line that the comment is on and the line that the
 * compiler is expected to report the error/warning on. It can be omitted together with the '@',
 * in which case it is set to 0, meaning the error/warning is expected on the same line. In case
 * the error/warning has no source line reported, the text NO SOURCE should be written in place
 * of OFFSET, which makes the check not require any source line.
 *
 * REGEXP is the regular expression for the expected error/warning message.
 *
 * The expected error/warning messages are EMCAScript regular expressions that messages reported
 * during compilation will be matched against. If the source line number is required, the repored
 * message has to include it. If the reported message has multiple locations, the specified source
 * line can be any of them.
 *
 * If there is at least one error check in the program, the compiler will return a success
 * return code (0) if each reported error has been matched with one error check and there are no
 * unmatched error checks left at the end of the compilation, otherwise the compiler will report
 * a failure. Warnings are less strict and will cause a failure only if there are unmatched warning
 * checks left at the end of the compilation, but there can be other warnings that are not
 * listed in checks.
 *
 * Example checks:
 * - Check that requires the same source location as the comment:
 * // expect error: "The action increment indexes .* with .* but it also indexes .* with .*\."
 *
 * - Check that requires the preceding line as the source location:
 * // expect warning@-1: "field list cannot be empty"
 *
 * - Check that doesn't require a source location:
 * // expect error@NO SOURCE: "0, "PHV allocation failed"
 */

void collect_diagnostic_checks(BfErrorReporter &reporter, BFN_Options &options);

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_LOGGING_COLLECT_DIAGNOSTIC_CHECKS_H_ */
