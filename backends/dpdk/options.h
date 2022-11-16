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

#ifndef BACKENDS_DPDK_OPTIONS_H_
#define BACKENDS_DPDK_OPTIONS_H_

#include "backends/dpdk/midend.h"

namespace DPDK {

class DpdkOptions : public CompilerOptions {
 public:
    cstring bfRtSchema = "";
    // file to output to
    cstring outputFile = nullptr;
    // file to ouput TDI Json to
    cstring tdiFile = "";
    // file to ouput context Json to
    cstring ctxtFile = "";
    // read from json
    bool loadIRFromJson = false;
    // Enable/Disable Egress pipeline in psa
    bool enableEgress = false;

    DpdkOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char*) {
                listMidendPasses = true;
                DPDK::DpdkMidEnd midEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[Dpdk back-end] Lists exact name of all midend passes.\n");
        registerOption(
            "--enableEgress", nullptr,
            [this](const char*) {
                enableEgress = true;
                return true;
            },
            "[Dpdk back-end] Enable egress pipeline's codegen\n", OptionFlags::Hide);

        registerOption(
            "--bf-rt-schema", "file",
            [this](const char* arg) {
                bfRtSchema = arg;
                return true;
            },
            "Generate and write BF-RT JSON schema to the specified file");
        registerOption(
            "-o", "outfile",
            [this](const char* arg) {
                outputFile = arg;
                return true;
            },
            "Write output to outfile");
        registerOption(
            "--tdi", "file",
            [this](const char* arg) {
                tdiFile = arg;
                return true;
            },
            "Generate and write TDI JSON to the specified file");
        registerOption(
            "--context", "file",
            [this](const char* arg) {
                ctxtFile = arg;
                return true;
            },
            "Generate and write context JSON to the specified file");
        registerOption(
            "--fromJSON", "file",
            [this](const char* arg) {
                loadIRFromJson = true;
                file = arg;
                return true;
            },
            "Use IR representation from JsonFile dumped previously,"
            "the compilation starts with reduced midEnd.");
    }

    /// Process the command line arguments and set options accordingly.
    std::vector<const char*>* process(int argc, char* const argv[]) override;

    const char* getIncludePath() override;
};

using DpdkContext = P4CContextWithOptions<DpdkOptions>;

}  // namespace DPDK

#endif /* BACKENDS_DPDK_OPTIONS_H_ */
