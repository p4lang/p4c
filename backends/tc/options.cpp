/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "options.h"

#include <charconv>
#include <cstring>
#include <limits>
#include <string>

#include "absl/strings/ascii.h"
#include "lib/error.h"

namespace P4::TC {
namespace {

// Parse a decimal unsigned value: digits only, the whole argument consumed,
// and the result inside [0, maxValue]. Signs, whitespace, radix prefixes and
// suffixes are rejected.
bool parseDecimalUnsigned(const char *arg, unsigned maxValue, unsigned &value) {
    const char *end = arg + std::strlen(arg);
    unsigned parsed = 0;
    auto result = std::from_chars(arg, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end || parsed > maxValue) return false;
    value = parsed;
    return true;
}

// The cache name is only a stem: the emitter prefixes it before handing it to
// refMap->newName(), which owns the reserved namespace and the collisions.
bool isCIdentifierStem(const std::string &name) {
    if (name.empty() || !(absl::ascii_isalpha(name[0]) || name[0] == '_')) return false;
    for (unsigned char c : name) {
        if (!absl::ascii_isalnum(c) && c != '_') return false;
    }
    return true;
}

// Split START:END for --tc-outline-parser-range.
bool parseStateRange(const char *arg, std::string &start, std::string &end) {
    std::string range(arg);
    auto colon = range.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == range.size()) return false;
    start = range.substr(0, colon);
    end = range.substr(colon + 1);
    return true;
}

}  // namespace

TCOptions::TCOptions() {
    registerOption(
        "-o", "output Directory",
        [this](const char *arg) {
            outputFolder = arg;
            return true;
        },
        "Write pipeline template, introspection json and C output to given directory");
    registerOption(
        "-g", nullptr,
        [this](const char *) {
            DebugOn = true;
            return true;
        },
        "Generates debug information");
    registerOption(
        "--trace", nullptr,
        [this](const char *) {
            emitTraceMessages = true;
            return true;
        },
        "Generate tracing messages of packet processing");
    registerOption(
        "--xdp2tc", "MODE",
        [this](const char *arg) {
            if (!strcmp(arg, "meta")) {
                xdp2tcMode = XDP2TC_META;
            } else if (!strcmp(arg, "head")) {
                xdp2tcMode = XDP2TC_HEAD;
            } else if (!strcmp(arg, "cpumap")) {
                xdp2tcMode = XDP2TC_CPUMAP;
            } else {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Unknown --xdp2tc mode '%1%'; expected one of: meta, head, "
                            "cpumap",
                            arg);
                return false;
            }
            return true;
        },
        "Select the mode used to pass metadata from XDP to TC "
        "(possible values: meta, head, cpumap).");
    registerOption(
        "--tc-outline-parser-range", "START:END",
        [this](const char *arg) {
            std::string start, end;
            if (!parseStateRange(arg, start, end)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-range expects START:END, got '%1%'", arg);
                return false;
            }
            if (outlineParserStart.empty()) {
                outlineParserStart = start;
                outlineParserEnd = end;
            } else if (outlineParserStart2.empty()) {
                outlineParserStart2 = start;
                outlineParserEnd2 = end;
            } else {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-range takes at most two ranges, got '%1%'", arg);
                return false;
            }
            return true;
        },
        "Outline a contiguous parser-state range into a noinline helper. "
        "Repeat the option for a second range.");
    registerOption(
        "--tc-outline-deparser", nullptr,
        [this](const char *) {
            outlineDeparser = true;
            return true;
        },
        "Outline deparser header emission into noinline helpers.");
    registerOption(
        "--tc-outline-deparser-max", "BYTES",
        [this](const char *arg) {
            constexpr unsigned maxBytes = std::numeric_limits<int>::max() / 8;
            unsigned value = 0;
            if (!parseDecimalUnsigned(arg, maxBytes, value)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-deparser-max expects decimal BYTES in [0, %1%], "
                            "got '%2%'",
                            maxBytes, arg);
                return false;
            }
            outlineDeparserMax = value;
            return true;
        },
        "Bound deparser helper offsets and output length in bytes.");
    registerOption(
        "--tc-outline-parser-no-hdr2", nullptr,
        [this](const char *) {
            outlineNoHdr2 = true;
            return true;
        },
        "Omit the header pointer from the second outlined helper.");
    registerOption(
        "--tc-outline-parser-offset-max", "BYTES",
        [this](const char *arg) {
            constexpr unsigned maxBytes = std::numeric_limits<int>::max() / 8;
            unsigned value = 0;
            if (!parseDecimalUnsigned(arg, maxBytes, value)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-offset-max expects decimal BYTES in [0, "
                            "%1%], got '%2%'",
                            maxBytes, arg);
                return false;
            }
            outlineOffsetMax = value;
            outlineOffsetMaxSet = true;
            return true;
        },
        "Bound the outlined helper entry offset in bytes.");
    registerOption(
        "--tc-outline-parser-local", "FIELD:LOCAL:MAX",
        [this](const char *arg) {
            std::string spec(arg);
            auto c1 = spec.find(':');
            auto c2 = c1 == std::string::npos ? std::string::npos : spec.find(':', c1 + 1);
            if (c1 == std::string::npos || c2 == std::string::npos || c1 == 0 || c1 + 1 == c2 ||
                c2 + 1 == spec.size()) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-local expects FIELD:LOCAL:MAX, got '%1%'", arg);
                return false;
            }
            auto max = spec.substr(c2 + 1);
            unsigned maxValue = 0;
            if (!parseDecimalUnsigned(max.c_str(), std::numeric_limits<unsigned>::max(),
                                      maxValue)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-local expects a decimal MAX in [0, %1%], "
                            "got '%2%'",
                            std::numeric_limits<unsigned>::max(), max);
                return false;
            }
            auto local = spec.substr(c1 + 1, c2 - c1 - 1);
            if (!isCIdentifierStem(local)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-local LOCAL must be an identifier stem, got "
                            "'%1%'",
                            local);
                return false;
            }
            outlineLocalField = spec.substr(0, c1);
            outlineLocalName = local;
            outlineLocalMax = maxValue;
            return true;
        },
        "Cache FIELD in LOCAL for the last outlined parser range.");
    registerOption(
        "--tc-outline-parser-bound", "FIELD:MAX",
        [this](const char *arg) {
            std::string range(arg);
            auto colon = range.find(':');
            if (colon == std::string::npos || colon == 0 || colon + 1 == range.size()) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-bound expects FIELD:MAX, got '%1%'", arg);
                return false;
            }
            auto max = range.substr(colon + 1);
            unsigned maxValue = 0;
            if (!parseDecimalUnsigned(max.c_str(), std::numeric_limits<unsigned>::max(),
                                      maxValue)) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--tc-outline-parser-bound expects a decimal uint32 MAX in "
                            "[0, %1%], got '%2%'",
                            std::numeric_limits<unsigned>::max(), max);
                return false;
            }
            outlineParserBoundField = range.substr(0, colon);
            outlineParserBoundValue = maxValue;
            return true;
        },
        "Bound a metadata field at the outlined parser-region entry.");
    registerOption(
        "--num-timer-profiles", "profiles",
        [this](const char *arg) {
            char *end = nullptr;
            long value = strtol(arg, &end, 0);
            if (*end != 0 || value <= 0 ||
                static_cast<unsigned long>(value) >
                    static_cast<unsigned long>(std::numeric_limits<unsigned>::max())) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--num-timer-profiles expects a positive integer, got '%1%'", arg);
                return false;
            }
            timerProfiles = static_cast<unsigned>(value);
            return true;
        },
        "Defines the number of timer profiles. Default is 4.");
}

}  // namespace P4::TC
