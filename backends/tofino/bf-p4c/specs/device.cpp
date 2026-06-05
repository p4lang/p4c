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

#include "backends/tofino/bf-p4c/specs/device.h"

#include <algorithm>

#include "lib/error.h"

Device *Device::instance_ = nullptr;
int Device::numStagesRuntimeOverride_ = 0;

void Device::init(cstring name, Options options) {
    instance_ = nullptr;

    std::string lower_name(name);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (lower_name == "tofino")
        instance_ = new TofinoDevice(options);
    else if (lower_name == "tofino2")
        instance_ = new JBayDevice(options);
#if BAREFOOT_INTERNAL
    else if (lower_name == "tofino2h")
        instance_ = new JBayHDevice(options);
#endif /* BAREFOOT_INTERNAL */
    else if (lower_name == "tofino2m")
        instance_ = new JBayMDevice(options);
    else if (lower_name == "tofino2u")
        instance_ = new JBayUDevice(options);
    else if (lower_name == "tofino2a0")
        instance_ = new JBayA0Device(options);
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

const Device::StatefulAluSpec &TofinoDevice::getStatefulAluSpec() const {
    static const Device::StatefulAluSpec spec = {/* .CmpMask = */ false,
                                                 /* .CmpUnits = */ {"lo"_cs, "hi"_cs},
                                                 /* .MaxSize = */ 32,
                                                 /* .MaxDualSize = */ 64,
                                                 /* .MaxPhvInputWidth = */ 32,
                                                 /* .MaxInstructions = */ 4,
                                                 /* .MaxInstructionConstWidth = */ 4,
                                                 /* .MinInstructionConstValue = */ -8,
                                                 /* .MaxInstructionConstValue = */ 7,
                                                 /* .OutputWords = */ 1,
                                                 /* .DivModUnit = */ false,
                                                 /* .FastClear = */ false,
                                                 /* .MaxRegfileRows = */ 4};
    return spec;
}

const Device::StatefulAluSpec &JBayDevice::getStatefulAluSpec() const {
    static const Device::StatefulAluSpec spec = {
        /* .CmpMask = */ true,
        /* .CmpUnits = */ {"cmp0"_cs, "cmp1"_cs, "cmp2"_cs, "cmp3"_cs},
        /* .MaxSize = */ 128,
        /* .MaxDualSize = */ 128,
        /* .MaxPhvInputWidth = */ 64,
        /* .MaxInstructions = */ 4,
        /* .MaxInstructionConstWidth = */ 4,
        /* .MinInstructionConstValue = */ -8,
        /* .MaxInstructionConstValue = */ 7,
        /* .OutputWords = */ 4,
        /* .DivModUnit = */ true,
        /* .FastClear = */ true,
        /* .MaxRegfileRows = */ 4};
    return spec;
}

const Device::GatewaySpec &TofinoDevice::getGatewaySpec() const {
    static const Device::GatewaySpec spec = {
        /* .PhvBytes = */ 4,
        /* .HashBits = */ 12,
        /* .PredicateBits = */ 0,
        /* .MaxRows = */ 4,
        /* .SupportXor = */ true,
        /* .SupportRange = */ true,
        /* .ExactShifts = */ 1,
        /* .ByteSwizzle = */ true,
        /* .PerByteMatch = */ 0,
        /* .XorByteSlots = */ 0xf0,
    };
    return spec;
}
const Device::GatewaySpec &JBayDevice::getGatewaySpec() const {
    static const Device::GatewaySpec spec = {
        /* .PhvBytes = */ 4,
        /* .HashBits = */ 12,
        /* .PredicateBits = */ 0,
        /* .MaxRows = */ 4,
        /* .SupportXor = */ true,
        /* .SupportRange = */ true,
        /* .ExactShifts = */ 5,
        /* .ByteSwizzle = */ true,
        /* .PerByteMatch = */ 0,
        /* .XorByteSlots = */ 0xf0,
    };
    return spec;
}
