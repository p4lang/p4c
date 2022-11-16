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

#ifndef _BACKENDS_EBPF_EBPFOPTIONS_H_
#define _BACKENDS_EBPF_EBPFOPTIONS_H_

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
    // XDP2TC mode for PSA-eBPF
    enum XDP2TC xdp2tcMode = XDP2TC_NONE;
    // maximum number of unique ternary masks
    unsigned int maxTernaryMasks = 128;

    EbpfOptions();

    void calculateXDP2TCMode() {
        if (arch != "psa") {
            return;
        }

        if (xdp2tcMode == XDP2TC_NONE) {
            std::cout << "Setting XDP2TC 'meta' mode by default." << std::endl;
            // Use 'meta' mode by default.
            xdp2tcMode = XDP2TC_META;
        }
        BUG_CHECK(xdp2tcMode != XDP2TC_NONE, "xdp2tc mode should not be set to NONE, bug?");
    }
};

using EbpfContext = P4CContextWithOptions<EbpfOptions>;

#endif /* _BACKENDS_EBPF_EBPFOPTIONS_H_ */
