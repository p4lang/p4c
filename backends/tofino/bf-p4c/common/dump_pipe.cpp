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
