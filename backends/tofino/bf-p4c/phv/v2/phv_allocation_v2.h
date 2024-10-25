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

#ifndef BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_
#define BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_

#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "lib/cstring.h"

namespace PHV {
namespace v2 {

class PhvAllocation : public Visitor {
    const PhvKit &kit_i;
    const MauBacktracker &mau_bt_i;
    PhvInfo &phv_i;
    int pipe_id_i = -1;

    const IR::Node *apply_visitor(const IR::Node *root, const char *name = 0) override;

 public:
    PhvAllocation(const PhvKit &kit, const MauBacktracker &mau, PhvInfo &phv)
        : kit_i(kit), mau_bt_i(mau), phv_i(phv) {}
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_PHV_ALLOCATION_V2_H_ */
