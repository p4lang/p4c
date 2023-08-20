/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_EBPF_EBPFOPTIONS_H_
#define BACKENDS_EBPF_EBPFOPTIONS_H_

#include <getopt.h>

#include "frontends/common/options.h"

enum XDP2TC { XDP2TC_NONE, XDP2TC_META, XDP2TC_HEAD, XDP2TC_CPUMAP };

class EbpfOptions : public CompilerOptions {
 public:
    // file to output to
    cstring outputFile = nullptr;
    // read from json
    bool loadIRFromJson = false;
    // Externs generation
    bool emitExterns = false;
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

    EbpfOptions();

    void calculateXDP2TCMode() {
        if (arch != "psa") {
            return;
        }

        if (generateToXDP && xdp2tcMode == XDP2TC_META) {
            std::cerr
                << "XDP2TC 'meta' mode cannot be used if XDP is enabled. "
                   "Falling back to 'head' mode. For more information see "
                   "https://github.com/p4lang/p4c/blob/main/backends/ebpf/psa/README.md#xdp2tc-mode"
                << std::endl;
            xdp2tcMode = XDP2TC_HEAD;
        } else if (generateToXDP && xdp2tcMode == XDP2TC_NONE) {
            // use 'head' mode by default; it's the safest option.
            std::cout << "Setting XDP2TC 'head' mode by default for XDP-based hook." << std::endl;
            xdp2tcMode = XDP2TC_HEAD;
        } else if (!generateToXDP && xdp2tcMode == XDP2TC_NONE) {
            std::cout << "Setting XDP2TC 'meta' mode by default for TC-based hook." << std::endl;
            // For TC, use 'meta' mode by default.
            xdp2tcMode = XDP2TC_META;
        }
        BUG_CHECK(xdp2tcMode != XDP2TC_NONE, "xdp2tc mode should not be set to NONE, bug?");
    }
};

using EbpfContext = P4CContextWithOptions<EbpfOptions>;

#endif /* BACKENDS_EBPF_EBPFOPTIONS_H_ */
