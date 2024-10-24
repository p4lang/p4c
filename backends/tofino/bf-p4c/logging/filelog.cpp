/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
