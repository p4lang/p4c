/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TC_OPTIONS_H_
#define BACKENDS_TC_OPTIONS_H_

#include <string>

#include "backends/ebpf/ebpfOptions.h"
#include "frontends/common/options.h"

namespace P4::TC {

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
    bool outlineDeparser = false;
    unsigned outlineDeparserMax = 1522;
    std::string outlineParserBoundField;
    unsigned outlineParserBoundValue = 0;

    TCOptions();
};

using TCContext = P4CContextWithOptions<TCOptions>;

}  // namespace P4::TC

#endif /* BACKENDS_TC_OPTIONS_H_ */
