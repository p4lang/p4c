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
