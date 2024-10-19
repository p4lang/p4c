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

#include "device.h"

#include <algorithm>

#include "lib/error.h"

Device *Device::instance_ = nullptr;
int Device::numStagesRuntimeOverride_ = 0;

void Device::init(cstring name) {
    instance_ = nullptr;

    std::string lower_name(name);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (lower_name == "tofino")
        instance_ = new TofinoDevice();
    else if (lower_name == "tofino2")
        instance_ = new JBayDevice();
#if BAREFOOT_INTERNAL
    else if (lower_name == "tofino2h")
        instance_ = new JBayHDevice();
#endif /* BAREFOOT_INTERNAL */
    else if (lower_name == "tofino2m")
        instance_ = new JBayMDevice();
    else if (lower_name == "tofino2u")
        instance_ = new JBayUDevice();
    else if (lower_name == "tofino2a0")
        instance_ = new JBayA0Device();
    else
        BUG("Unknown device %s", name);
}

void Device::overrideNumStages(int num) {
    int realNumStages = Device::get().getNumStages();
    if (num < 0 || num > realNumStages) {
        ::error("Trying to override mau stages count to %d but device is capped to %d.", num,
                realNumStages);
        return;
    }

    numStagesRuntimeOverride_ = num;
}
