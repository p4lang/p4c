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

#include "bf_error_reporter.h"

#include <fstream>
#include <numeric>
#include <regex>

#include "bf-p4c-options.h"
#include "event_logger.h"

static bool has_output_already_been_silenced = false;
void reset_has_output_already_been_silenced() {  //  should this do more?
    has_output_already_been_silenced = false;
}
bool get_has_output_already_been_silenced() { return has_output_already_been_silenced; }

using namespace std;

void redirect_all_standard_outputs_to_dev_null() {
    // no good ctors for this acc. to <https://m.cplusplus.com/reference/fstream/filebuf/open/> :-(
    static filebuf devNull;
    devNull.open("/dev/null", ios::out | ios::app);

    cerr.rdbuf(&devNull);
    clog.rdbuf(&devNull);
    cout.rdbuf(&devNull);

    has_output_already_been_silenced = true;
}

void check_the_error_count_and_act_accordingly(const BfErrorReporter &BFER) {
    const auto EC{BFER.getErrorCount()};
    if (Log::Detail::verbosity > 0) {
        // just  "if (Log::verbose)" wasn`t good enough: too much spew;
        //   "if ((*Log::verbose) > 1)" was even _worse_
        cerr << "Error count: " << EC << " [cerr]\n";
        cout << "Error count: " << EC << " [cout]\n";
    }
    if (EC >
        BackendOptions().inclusive_max_errors_before_enforcing_silence_other_than_the_summary) {
        cerr << "INFO: suppressing all non-summary non-file output from this point on." << endl;
        redirect_all_standard_outputs_to_dev_null();
    }
}  //  end of “check_the_error_count_and_act_accordingly”

void BfErrorReporter::emit_message(const ErrorMessage &msg) {
    check_the_error_count_and_act_accordingly(*this);

    if (msg.type == ErrorMessage::MessageType::Error) {
        EventLogger::get().error(msg);
    } else if (msg.type == ErrorMessage::MessageType::Warning) {
        EventLogger::get().warning(msg);
    }
    ErrorReporter::emit_message(msg);

#if BAREFOOT_INTERNAL
    bool match = match_checks(msg);
    if (has_error_checks && msg.type == ErrorMessage::MessageType::Error && !match) {
        unexpected_errors.push_back(msg.toString());
    }
#endif
}

void BfErrorReporter::emit_message(const ParserErrorMessage &msg) {
    check_the_error_count_and_act_accordingly(*this);

    EventLogger::get().parserError(msg);
    ErrorReporter::emit_message(msg);

    check_the_error_count_and_act_accordingly(*this);

#if BAREFOOT_INTERNAL
    const auto str = msg.toString();
    bool match = match_check(ErrorMessage::MessageType::Error, msg.location.line, str);
    if (has_error_checks && !match) {
        unexpected_errors.push_back(str);
    }
#endif
}

void BfErrorReporter::add_check(ErrorMessage::MessageType type, const std::string &msg) {
    add_check(type, NO_SOURCE, msg);
}

void BfErrorReporter::add_check(ErrorMessage::MessageType type, int line, const std::string &msg) {
    BUG_CHECK(
        type == ErrorMessage::MessageType::Warning || type == ErrorMessage::MessageType::Error,
        "Invalid type %1%, should be Warning or Error.", std::size_t(type));
    LOG1("Adding check: " << (type == ErrorMessage::MessageType::Error ? "error" : "warning")
                          << " line " << line << " msg \"" << msg << "\"");

    auto &v = checks[line];
    v.emplace_back(msg, type);
    if (type == ErrorMessage::MessageType::Error) {
        has_error_checks = true;
    }
}

bool BfErrorReporter::match_checks(const ErrorMessage &msg) {
    const auto str = msg.toString();
    if (msg.locations.empty()) {
        return match_check(msg.type, NO_SOURCE, str);
    }
    for (const auto &loc : msg.locations) {
        // If an error/warning is reported, return true if it matches at least one check.
        if (match_check(msg.type, loc.toPosition().sourceLine, str)) {
            return true;
        }
    }

    return false;
}

bool BfErrorReporter::match_check(ErrorMessage::MessageType type, int line,
                                  const std::string &msg) {
    if (const auto it = checks.find(line); it != checks.end()) {
        for (auto &check : it->second) {
            LOG5("Matching check on line " << line << ": " << check.msg << " (" << check.matched
                                           << ")");
            if (check.type == type && !check.matched) {
                std::regex expected(check.msg);
                if (std::regex_search(msg, expected)) {
                    LOG1("Matched check: " << check.msg);
                    check.matched = true;
                    return true;
                }
            }
        }
    }

    return false;
}

BfErrorReporter::CheckResult BfErrorReporter::verify_checks() const {
    if (checks.empty()) {
        LOG1("No checks");
        return CheckResult::NO_TESTS;
    }

    bool success = true;
    // No early returns because we want to report all unexpected errors and unmatched checks.
    if (!unexpected_errors.empty()) {
        std::string msg = "The following errors were not expected:\n";
        msg = std::accumulate(unexpected_errors.begin(), unexpected_errors.end(), msg);
        error(ErrorType::ERR_UNEXPECTED, msg.c_str());
    }
    for (const auto &[line, v] : checks) {
        for (const auto &check : v) {
            if (!check.matched) {
                error(ErrorType::ERR_NOT_FOUND,
                      "Unmatched check: Expected %1% message \"%2%\"%3% not reported.",
                      (check.type == ErrorMessage::MessageType::Error) ? "error" : "warning",
                      check.msg, (line == NO_SOURCE) ? "" : " at line " + std::to_string(line));
                success = false;
            }
        }
    }
    success = success && unexpected_errors.empty();

    if (success) {
        LOG1("Check verification successful");
    }
    return success ? CheckResult::SUCCESS : CheckResult::FAILURE;
}
