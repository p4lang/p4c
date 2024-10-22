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

#include <iostream>

#include "ir/dump.h"
#include "lib/gc.h"
#include "utils.h"

using namespace P4;

#if BAREFOOT_INTERNAL
bool DumpPipe::preorder(const IR::Node *pipe) {
    if (LOGGING(1)) {
        if (heading) {
            LOG1("-------------------------------------------------");
            LOG1(heading);
            LOG1("-------------------------------------------------");
            size_t maxMem = 0;
            size_t memUsed = gc_mem_inuse(&maxMem) / (1024 * 1024);
            maxMem = maxMem / (1024 * 1024);
            LOG1("*** mem in use = " << memUsed << "MB, heap size = " << maxMem << "MB");
        }
        if (LOGGING(2))
            dump(::Log::Detail::fileLogOutput(__FILE__), pipe);
        else
            LOG1(*pipe);
    }
    return false;
}
#endif  // BAREFOOT_INTERNAL
