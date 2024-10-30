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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_ADJUST_BYTE_COUNT_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_ADJUST_BYTE_COUNT_H_

#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/mau_visitor.h"
namespace BFN {

using namespace P4;

// This pass sets up the adjust byte count value on a counter or a meter.
// It first scans the IR for any counter/meter extern objects and checks if
// their respective methods (count/execute) have an 'adjust_byte_count'
// argument. For all method calls for the same counter/meter the argument value
// is verified to be the same. This is required the counter/meter can only set a
// single value as adjust byte count. Post verification the value is stored in
// the extern object
class AdjustByteCountSetup : public PassManager {
    std::map<const IR::MAU::AttachedMemory *, const int64_t> adjust_byte_counts;

    class Scan : public MauInspector {
        AdjustByteCountSetup &self;

     public:
        explicit Scan(AdjustByteCountSetup &self) : self(self) {}
        bool preorder(const IR::MAU::Primitive *prim) override;
    };

    class Update : public MauTransform {
        AdjustByteCountSetup &self;

     public:
        int get_bytecount(IR::MAU::AttachedMemory *am);
        const IR::MAU::Counter *preorder(IR::MAU::Counter *counter) override;
        const IR::MAU::Meter *preorder(IR::MAU::Meter *meter) override;
        explicit Update(AdjustByteCountSetup &self) : self(self) {}
    };

 public:
    AdjustByteCountSetup();
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_ADJUST_BYTE_COUNT_H_ */
