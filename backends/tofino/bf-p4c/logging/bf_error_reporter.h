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

#ifndef _EXTENSIONS_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_
#define _EXTENSIONS_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_

#include "lib/error_reporter.h"
#include "ir/ir.h"

bool   get_has_output_already_been_silenced();
void reset_has_output_already_been_silenced();

class BfErrorReporter : public ErrorReporter {
    struct Check {
        std::string msg;
        ErrorMessage::MessageType type;
        bool matched;

        Check(const std::string &msg, ErrorMessage::MessageType type)
            : msg(msg), type(type), matched(false) {}
    };

    static constexpr int NO_SOURCE = 0;
    // source line to expected warning/error messages on that line
    std::unordered_map<int, std::vector<Check>> checks;
    std::vector<std::string> unexpected_errors;
    bool has_error_checks = false;
    bool match_checks(const ErrorMessage &msg);
    bool match_check(ErrorMessage::MessageType type, int line, const std::string &msg);

 protected:
    void emit_message(const ErrorMessage &msg) override;

    void emit_message(const ParserErrorMessage &msg) override;

 public:
    enum class CheckResult { NO_TESTS, SUCCESS, FAILURE };

    void increment_the_error_count() { ++this->errorCount; }

    /**
     * @brief Adds a check with no specified source code line.
     * 
     * @param type error or warning
     * @param msg expected regex
     */
    void add_check(ErrorMessage::MessageType type, const std::string &msg);
    /**
     * @brief Adds a check which requires that an error/warning message includes the specified
     * regex as well as the specified line number.
     *
     * @param type error or warning
     * @param line source location that the error/warning has to include, value of 0 means that no
     * source location is required
     * @param msg expected regex
     */
    void add_check(ErrorMessage::MessageType type, int line, const std::string &msg);

    /**
     * @brief Verify error/warning checks in the program. During compilation, if an error/warning
     * message matches the check message regex and its source line, the check is considered
     * to be successful.
     *
     * This method should be called at the end of compilation. It will report any unmatched
     * checks as errors as well as any additional errors that weren't matched to a check.
     * If there are no checks in the program, the method does nothing and returns NO_TESTS.
     *
     * @return CheckResult
     */
    CheckResult verify_checks() const;
};

#endif  /* _EXTENSIONS_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_ */
