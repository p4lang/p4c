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

#include "collect_diagnostic_checks.h"

#include <optional>
#include <regex>

namespace BFN {

static std::optional<std::string> get_file_contents(BFN_Options &options) {
    auto preprocessorResult = options.preprocess();
    if (!preprocessorResult.has_value()) {
        // Return std::nullopt if preprocess fails
        return std::nullopt;
    }

    FILE *file = preprocessorResult.value().get();

    constexpr std::size_t BUFFER_SIZE = 4096;
    std::string buffer;
    std::size_t read = 0;
    std::size_t len = 0;
    do {
        buffer.resize(len + BUFFER_SIZE, '\0');
        read = fread(buffer.data() + len, 1, BUFFER_SIZE, file);
        len += read;
    } while (read == BUFFER_SIZE);

    buffer.resize(len);
    return buffer;
}

static std::unordered_map<int, std::string> get_comments(const std::string contents) {
    std::unordered_map<int, std::string> comments;

    std::smatch match;
    int line_number = 1;
    std::size_t line_start = 0;

    // Matches # NUM or #line NUM.
    const std::regex line_regex("^#(?:line)?\\s*(\\d+).*");
    while (true) {
        const std::size_t line_end = contents.find('\n', line_start);
        const std::string &line = contents.substr(line_start, line_end - line_start);
        const std::size_t line_comment = line.find("//");
        const std::size_t block_comment = line.find("/*");
        if (line_comment != std::string::npos) {
            comments[line_number] = line.substr(line_comment, line_end - line_comment);
        }
        if (block_comment != std::string::npos) {
            const std::size_t end = contents.find("*/", line_start + block_comment);
            const std::size_t len = end - (line_start + block_comment);
            comments[line_number] = contents.substr(line_start + block_comment, len);
        }

        const auto end =
            line_end == std::string::npos ? contents.end() : contents.begin() + line_end;
        if (std::regex_search(contents.begin() + line_start, end, match, line_regex)) {
            const int groups = match.size();
            BUG_CHECK(groups == 2, "Invalid line regex match results: %1% groups instead of 2.",
                      groups);

            // match[0] is the whole line.

            line_number = std::stoi(match[1]);
        } else {
            ++line_number;
        }

        if (line_end == std::string::npos) {
            break;
        }
        line_start = line_end + 1;
    }

    return comments;
}

inline std::string to_lower(std::smatch::const_reference it) {
    std::string str = it.str();
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

void collect_diagnostic_checks(BfErrorReporter &reporter, BFN_Options &options) {
    const auto contents = get_file_contents(options);
    if (!contents.has_value()) return;
    const auto comments = get_comments(std::move(contents.value()));
    std::smatch match;

    /* Matches expect TYPE@OFFSET: "REGEXP",
     * where:
     * TYPE: "error" or "warning",
     * OFFSET: optional - (possibly negative) number or the text "NO SOURCE",
     * REGEXP: regular expression to match reported errors/warnings to, can be any string.
     * Allows for any whitespace on both side of '@' and ':', including none.
     */
    const std::regex check_regex(
        "expect (error|warning)(?:\\s*@\\s*([-+]?\\d+|NO SOURCE))?\\s*:\\s*\"((?:.|\\n)*)\"",
        std::regex_constants::icase);

    for (const auto &[line, comment] : comments) {
        if (std::regex_search(comment, match, check_regex)) {
            const int groups = match.size();
            BUG_CHECK(groups == 4, "Invalid check regex match results: %1% groups instead of 4.",
                      groups);

            // match[0] is the whole line.

            ErrorMessage::MessageType type = ErrorMessage::MessageType::Error;
            if (to_lower(match[1]) == "warning") {
                type = ErrorMessage::MessageType::Warning;
            }

            int offset = 0;
            bool requires_source_location = true;
            const auto &offset_match = match[2];
            if (offset_match.matched) {
                if (to_lower(offset_match).find("no source") != std::string::npos) {
                    requires_source_location = false;
                } else {
                    offset = std::stoi(offset_match.str());
                }
            }

            const auto &msg = match[3];

            if (requires_source_location) {
                reporter.add_check(type, line + offset, msg);
            } else {
                reporter.add_check(type, msg);
            }
        }
    }
}

}  // namespace BFN
