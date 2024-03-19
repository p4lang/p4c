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
    cstring outputFolder = nullptr;
    bool DebugOn = false;
    // tracing eBPF code execution
    bool emitTraceMessages = false;
    // XDP2TC mode for PSA-eBPF
    enum XDP2TC xdp2tcMode = XDP2TC_META;
    unsigned timerProfiles = 4;

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
                }
                return true;
            },
            "Select the mode used to pass metadata from XDP to TC "
            "(possible values: meta, head, cpumap).");
        registerOption(
            "--num-timer-profiles", "profiles",
            [this](const char *arg) {
                timerProfiles = std::atoi(arg);
                return true;
            },
            "Defines the number of timer profiles. Default is 4.");
    }
};

using TCContext = P4CContextWithOptions<TCOptions>;

}  // namespace TC

#endif /* BACKENDS_TC_OPTIONS_H_ */
