/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "filelog.h"

std::set<cstring> Logging::FileLog::filesWritten;

// static
const cstring &Logging::FileLog::name2type(cstring logName) {
    static const cstring unknown("Unknown");
    static const std::map<cstring, cstring> logNames2Type = {
        {"clot_allocation"_cs, "parser"_cs},
        {"decaf"_cs, "parser"_cs},
        {"flexible_packing"_cs, "parser"_cs},
        {"ixbar"_cs, "mau"_cs},
        {"parser.characterize"_cs, "parser"_cs},
        {"parser"_cs, "parser"_cs},
        {"phv_allocation"_cs, "phv"_cs},
        {"phv_optimization"_cs, "phv"_cs},
        {"phv_incremental_allocation"_cs, "phv"_cs},
        {"phv_trivial_allocation"_cs, "phv"_cs},
        {"phv_greedy_allocation"_cs, "phv"_cs},
        {"table_"_cs, "mau"_cs},
        {"pragmas"_cs, "phv"_cs},
        {"live_range_split"_cs, "phv"_cs},
        {"backend_passes"_cs, "text"_cs}};

    for (auto &logType : logNames2Type)
        if (logName.startsWith(logType.first.string_view())) return logType.second;
    return unknown;
}
