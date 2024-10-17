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

#ifndef BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_
#define BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_

#include "lib/cstring.h"

#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/utils/utils.h"

namespace PHV {
namespace v2 {

class PhvAllocation : public Visitor {
    const PhvKit& kit_i;
    const MauBacktracker& mau_bt_i;
    PhvInfo& phv_i;
    int pipe_id_i = -1;

    const IR::Node *apply_visitor(const IR::Node* root, const char *name = 0) override;

 public:
    PhvAllocation(const PhvKit& kit, const MauBacktracker& mau, PhvInfo& phv)
        : kit_i(kit), mau_bt_i(mau), phv_i(phv) {}
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_ */
