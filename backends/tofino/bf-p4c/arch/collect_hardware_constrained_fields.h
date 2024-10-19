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

#ifndef BF_P4C_ARCH_COLLECT_HARDWARE_CONSTRAINED_FIELDS_H_
#define BF_P4C_ARCH_COLLECT_HARDWARE_CONSTRAINED_FIELDS_H_

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace BFN {

// This class add hardware constrained fields to pipe->thread[gress].hw_constrained_fields. This is
// a honeypot for Alias pass, so that Alias pass can replace hardware constrained fields' Member
// with AliasMember or AliasSlice. And this part of information can be used in CollectPhvInfo and
// DisableAutoInitMetadata to set Field's properties properly for alias fields for hardware
// constrained fields.

class AddHardwareConstrainedFields : public Modifier {
 public:
    void postorder(IR::BFN::Pipe *) override;
};

class CollectHardwareConstrainedFields : public PassManager {
 public:
    CollectHardwareConstrainedFields() { addPasses({new AddHardwareConstrainedFields()}); }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_COLLECT_HARDWARE_CONSTRAINED_FIELDS_H_ */
