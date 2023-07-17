/* Copyright 2023 Intel Corporation

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

#include "backends/dpdk/tdiConf.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace DPDK {

void TdiBfrtConf::generate(DPDK::DpdkOptions &options) {
    if (options.outputFile.isNullOrEmpty()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "No output file provided. Unable to generate correct TDI builder config file.");
        return;
    }
    auto inputFile = std::filesystem::absolute(options.file.c_str());
    auto outFile = std::filesystem::absolute(options.outputFile.c_str());
    auto outDir = outFile.parent_path();
    auto tdiFile = std::filesystem::absolute(options.tdiBuilderConf.c_str());
    auto programName = inputFile.stem();

    if (options.bfRtSchema.isNullOrEmpty()) {
        options.bfRtSchema = (outDir / programName).replace_filename("json").c_str();
        ::warning(
            "BF-Runtime Schema file name not provided, but is required for the TDI builder "
            "configuration. Generating file %1%",
            options.bfRtSchema);
    }
    if (options.ctxtFile.isNullOrEmpty()) {
        options.ctxtFile = (outDir / "context.json").c_str();
        ::warning(
            "DPDK context file name not provided, but is required for the TDI builder "
            "configuration. Generating file %1%",
            options.ctxtFile);
    }

    auto contextFile = std::filesystem::absolute(options.ctxtFile.c_str());
    auto bfRtSchema = std::filesystem::absolute(options.bfRtSchema.c_str());
    // TODO: Ideally, this should be a template. We could use Inja, but this adds another
    // dependency.
    std::stringstream ss;
    ss << R"""({
    "chip_list": [
        {
            "chip_family": "dpdk"
        }
    ],
    "instance": 0,
    "p4_devices": [
        {
            "device-id": 0,
            "p4_programs": [
                {
)""";
    ss << R"""(                    "program-name" : )""";
    ss << programName << ",\n";
    ss << R"""(                    "bfrt-config" : )""";
    ss << bfRtSchema << ",\n";
    ss << R"""(                    "p4_pipelines": [
                        {
)""";
    ss << R"""(                            "p4_pipeline_name": "pipe",
                            "context": )""";
    ss << contextFile << ",";
    ss << R"""(
                            "config": )""";
    ss << outFile << ",";
    ss << R"""(
                            "pipe_scope": [
                                0,
                            ]
                        }
                    ]
                }
            ]
        }
    ]
}
)""";
    std::ofstream out;
    out.open(tdiFile);
    if (out.is_open()) {
        out << ss;
    } else {
        ::error(ErrorType::ERR_IO, "Could not open file: %1%", tdiFile.c_str());
        return;
    }
    out.close();
}
}  // namespace DPDK
