/*
Copyright 2020 Intel Corp.

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

#ifndef BACKENDS_DPDK_PSA_SWITCH_OPTIONS_H_
#define BACKENDS_DPDK_PSA_SWITCH_OPTIONS_H_

#include "backends/bmv2/common/options.h"
#include "backends/dpdk/midend.h"

namespace DPDK {

class DpdkOptions : public BMV2::BMV2Options {
  public:
    cstring bfRtSchema = "";

    DpdkOptions() {
        registerOption("--bf-rt-schema", "file",
                [this](const char *arg) { bfRtSchema = arg; return true; },
                "Generate and write BF-RT JSON schema to the specified file");
    }

    /// Process the command line arguments and set options accordingly.
    std::vector<const char*>* process(int argc, char* const argv[]) override;

    const char* getIncludePath() override;
};

using DpdkContext = P4CContextWithOptions<DpdkOptions>;

}; // namespace DPDK

#endif /* BACKENDS_DPDK_PSA_SWITCH_OPTIONS_H_ */
