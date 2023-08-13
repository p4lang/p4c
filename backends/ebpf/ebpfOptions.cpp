/*
Copyright 2022 VMware Inc.

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

#include "ebpfOptions.h"

#include <string.h>

#include <cstdlib>

#include "midend.h"

EbpfOptions::EbpfOptions() {
    langVersion = CompilerOptions::FrontendVersion::P4_16;
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outputFile = arg;
            return true;
        },
        "Write output to outfile");
    registerOption(
        "--listMidendPasses", nullptr,
        [this](const char *) {
            loadIRFromJson = false;
            listMidendPasses = true;
            EBPF::MidEnd midend;
            midend.run(*this, nullptr, outStream);
            exit(0);
            return false;
        },
        "[ebpf back-end] Lists exact name of all midend passes.\n");
    registerOption(
        "--fromJSON", "file",
        [this](const char *arg) {
            loadIRFromJson = true;
            file = arg;
            return true;
        },
        "Use IR representation from JsonFile dumped previously,"
        "the compilation starts with reduced midEnd.");
    registerOption(
        "--emit-externs", nullptr,
        [this](const char *) {
            emitExterns = true;
            return true;
        },
        "[ebpf back-end] Allow for user-provided implementation of extern functions.");
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
        "[psa only] Select the mode used to pass metadata from XDP to TC "
        "(possible values: meta, head, cpumap).");
    registerOption(
        "--table-caching", nullptr,
        [this](const char *) {
            enableTableCache = true;
            return true;
        },
        "[psa only] Enable caching entries for tables with lpm or ternary key");
    registerOption(
        "--xdp", nullptr,
        [this](const char *) {
            generateToXDP = true;
            return true;
        },
        "[psa only] Compile and generate the P4 prog for XDP hook");
}
