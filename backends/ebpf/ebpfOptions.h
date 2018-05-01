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

class EbpfOptions : public CompilerOptions {
 public:
    // file to output to
    cstring outputFile = nullptr;

    EbpfOptions() {
        langVersion = CompilerOptions::FrontendVersion::P4_16;
        registerOption("-o", "outfile",
                [this](const char* arg) { outputFile = arg; return true; },
                "Write output to outfile");
    }
};

using EbpfContext = P4CContextWithOptions<EbpfOptions>;

#endif /* _BACKENDS_EBPF_EBPFOPTIONS_H_ */
