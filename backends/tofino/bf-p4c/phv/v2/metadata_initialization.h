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

#ifndef BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_
#define BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_

#include <algorithm>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"

namespace PHV {

namespace v2 {
class MetadataInitialization : public PassManager {
    MauBacktracker &backtracker;

 public:
    MetadataInitialization(MauBacktracker &backtracker, const PhvInfo &phv, FieldDefUse &defuse);
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_ */
