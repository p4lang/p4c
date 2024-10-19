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
