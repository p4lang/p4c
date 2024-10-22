/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BACKENDS_TOFINO_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_
#define _BACKENDS_TOFINO_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_

#include "ir/ir.h"
#include "lib/error_reporter.h"

bool get_has_output_already_been_silenced();
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

#endif /* _BACKENDS_TOFINO_BF_P4C_LOGGING_BF_ERROR_REPORTER_H_ */
