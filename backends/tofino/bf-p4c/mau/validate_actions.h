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

#ifndef BF_P4C_MAU_VALIDATE_ACTIONS_H_
#define BF_P4C_MAU_VALIDATE_ACTIONS_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/visitor.h"

class PhvInfo;
class ReductionOrInfo;

class ValidateActions final : public MauInspector {
 private:
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    bool stop_compiler;
    bool phv_alloc;
    bool ad_alloc;
    bool warning_found = false;

    // true if action analysis finds a PHV allocation that violates MAU constraints.
    bool error_found = false;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Action *act) override;
    void end_apply() override;

 public:
    explicit ValidateActions(const PhvInfo &p, const ReductionOrInfo &ri, bool sc, bool pa, bool ad)
        : phv(p), red_info(ri), stop_compiler(sc), phv_alloc(pa), ad_alloc(ad) {}
};

#endif /* BF_P4C_MAU_VALIDATE_ACTIONS_H_ */
