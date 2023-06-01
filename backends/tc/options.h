/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_OPTIONS_H_
#define BACKENDS_TC_OPTIONS_H_

#include "backends/ebpf/ebpfOptions.h"
#include "frontends/common/options.h"
#include "lib/cstring.h"

namespace TC {

class TCOptions : public CompilerOptions {
 public:
    // file to output to
    cstring outputFile = nullptr;
    cstring cFile = nullptr;
    cstring introspecFile = nullptr;
    bool DebugOn = false;
    bool SetRules = false;
    unsigned int rules = 0;
    // tracing eBPF code execution
    bool emitTraceMessages = false;
    // generate program to XDP layer
    bool generateToXDP = false;
    // XDP2TC mode for PSA-eBPF
    enum XDP2TC xdp2tcMode = XDP2TC_NONE;
    // maximum number of unique ternary masks
    unsigned int maxTernaryMasks = 128;
    // Enable table cache for LPM and ternary tables
    bool enableTableCache = false;

    TCOptions() {
        registerOption(
            "-o", "outfile",
            [this](const char *arg) {
                outputFile = arg;
                return true;
            },
            "Write pipeline template output to outfile");
        registerOption(
            "-c", "cfile",
            [this](const char *arg) {
                cFile = arg;
                return true;
            },
            "Write c output to the given file");
        registerOption(
            "--set-rules-limit", "rules",
            [this](const char *arg) {
                SetRules = true;
                rules = atoi(arg);
                return true;
            },
            "Set the max rules limit for the pipeline");
        registerOption(
            "-g", nullptr,
            [this](const char *) {
                DebugOn = true;
                return true;
            },
            "Generates debug information");
        registerOption(
            "-i", "introspecFile",
            [this](const char *arg) {
                introspecFile = arg;
                return true;
            },
            "Write introspection json to the given file");
        registerOption(
            "--trace", nullptr,
            [this](const char *) {
                emitTraceMessages = true;
                return true;
            },
            "Generate tracing messages of packet processing");
        registerOption(
            "--max-ternary-masks", "MAX_TERNARY_MASKS",
            [this](const char *arg) {
                unsigned int parsed_val = std::strtoul(arg, nullptr, 0);
                if (parsed_val >= 2) this->maxTernaryMasks = parsed_val;
                return true;
            },
            "Set number of maximum possible masks for a ternary key"
            " in a single table");
        registerOption(
            "--xdp2tc", "MODE",
            [this](const char *arg) {
                if (!strcmp(arg, "meta")) {
                    xdp2tcMode = XDP2TC_META;
                } else if (!strcmp(arg, "head")) {
                    xdp2tcMode = XDP2TC_HEAD;
                } else if (!strcmp(arg, "cpumap")) {
                    xdp2tcMode = XDP2TC_CPUMAP;
                }
                return true;
            },
            "Select the mode used to pass metadata from XDP to TC "
            "(possible values: meta, head, cpumap).");
        registerOption(
            "--table-caching", nullptr,
            [this](const char *) {
                enableTableCache = true;
                return true;
            },
            "Enable caching entries for tables with lpm or ternary key");
        registerOption(
            "--xdp", nullptr,
            [this](const char *) {
                generateToXDP = true;
                return true;
            },
            "Compile and generate the P4 prog for XDP hook");
    }
    void calculateXDP2TCMode() {
        if (generateToXDP && xdp2tcMode == XDP2TC_META) {
            std::cerr << "XDP2TC 'meta' mode cannot be used if XDP is enabled. "
                         "Falling back to 'head' mode."
                      << std::endl;
            xdp2tcMode = XDP2TC_HEAD;
        } else if (generateToXDP && xdp2tcMode == XDP2TC_NONE) {
            // use 'head' mode by default; it's the safest option.
            xdp2tcMode = XDP2TC_HEAD;
        } else if (!generateToXDP && xdp2tcMode == XDP2TC_NONE) {
            // For TC, use 'meta' mode by default.
            xdp2tcMode = XDP2TC_META;
        }
        BUG_CHECK(xdp2tcMode != XDP2TC_NONE, "xdp2tc mode should not be set to NONE, bug?");
    }
};

using TCContext = P4CContextWithOptions<TCOptions>;

}  // namespace TC

#endif /* BACKENDS_TC_OPTIONS_H_ */
