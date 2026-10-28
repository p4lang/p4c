/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TC_OPTIONS_H_
#define BACKENDS_TC_OPTIONS_H_

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <set>
#include <string>

#include "backends/ebpf/ebpfOptions.h"
#include "frontends/common/options.h"
#include "lib/error.h"

namespace P4::TC {

// Parse a decimal unsigned value: digits only, the whole argument consumed,
// and the result inside [0, maxValue]. Signs, whitespace, radix prefixes and
// suffixes are rejected.
static bool parseDecimalUnsigned(const char *arg, unsigned maxValue, unsigned &value) {
    const char *end = arg + std::strlen(arg);
    unsigned parsed = 0;
    auto result = std::from_chars(arg, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end || parsed > maxValue) return false;
    value = parsed;
    return true;
}

static bool isCIdentifier(const std::string &name) {
    if (name.empty() || std::isdigit(static_cast<unsigned char>(name[0]))) return false;
    for (unsigned char c : name) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    static const std::set<std::string> keywords = {
        "alignas", "alignof", "asm", "auto", "bool", "break", "case", "char", "const",
        "constexpr", "continue", "default", "do", "double", "else", "enum", "extern",
        "false", "float", "for", "goto", "if", "inline", "int", "long", "nullptr",
        "register", "restrict", "return", "short", "signed", "sizeof", "static",
        "static_assert", "struct", "switch", "thread_local", "true", "typedef", "typeof",
        "typeof_unqual", "union", "unsigned", "void", "volatile", "while"};
    return !keywords.count(name) && name.rfind("__", 0) != 0 &&
           !(name[0] == '_' && name.size() > 1 &&
             std::isupper(static_cast<unsigned char>(name[1])));
}

class TCOptions : public CompilerOptions {
 public:
    // file to output to
    std::filesystem::path outputFolder;
    bool DebugOn = false;
    // tracing eBPF code execution
    bool emitTraceMessages = false;
    // XDP2TC mode for PSA-eBPF
    enum XDP2TC xdp2tcMode = XDP2TC_META;
    unsigned timerProfiles = 4;
    std::string outlineParserStart;
    std::string outlineParserEnd;
    std::string outlineParserStart2;
    std::string outlineParserEnd2;
    std::string outlineLocalField;
    std::string outlineLocalName;
    unsigned outlineLocalMax = 0;
    unsigned outlineOffsetMax = 0;
    bool outlineOffsetMaxSet = false;
    bool outlineNoHdr2 = false;
    std::string outlineParserBoundField;
    unsigned outlineParserBoundValue = 0;

    TCOptions() {
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
                std::string range(arg);
                auto colon = range.find(':');
                if (colon == std::string::npos || colon == 0 || colon + 1 == range.size()) {
                    ::P4::error(ErrorType::ERR_INVALID,
                                "--tc-outline-parser-range expects START:END, got '%1%'", arg);
                    return false;
                }
                outlineParserStart = range.substr(0, colon);
                outlineParserEnd = range.substr(colon + 1);
                return true;
            },
            "Outline a contiguous parser-state range into a noinline helper.");
        registerOption(
            "--tc-outline-parser-range2", "START:END",
            [this](const char *arg) {
                std::string range(arg);
                auto colon = range.find(':');
                if (colon == std::string::npos || colon == 0 || colon + 1 == range.size()) {
                    ::P4::error(ErrorType::ERR_INVALID,
                                "--tc-outline-parser-range2 expects START:END, got '%1%'", arg);
                    return false;
                }
                outlineParserStart2 = range.substr(0, colon);
                outlineParserEnd2 = range.substr(colon + 1);
                return true;
            },
            "Outline a second contiguous parser-state range.");
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
                                "--tc-outline-parser-local expects FIELD:LOCAL:MAX, got '%1%'",
                                arg);
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
                if (!isCIdentifier(local)) {
                    ::P4::error(ErrorType::ERR_INVALID,
                                "--tc-outline-parser-local LOCAL must be a C identifier, got "
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
};

using TCContext = P4CContextWithOptions<TCOptions>;

}  // namespace P4::TC

#endif /* BACKENDS_TC_OPTIONS_H_ */
