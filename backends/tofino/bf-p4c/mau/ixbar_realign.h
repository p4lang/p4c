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

#ifndef BF_P4C_MAU_IXBAR_REALIGN_H_
#define BF_P4C_MAU_IXBAR_REALIGN_H_

#include <array>

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "lib/safe_vector.h"

using namespace P4;

class IXBarVerify : public MauModifier {
    const PhvInfo &phv;
    const IR::MAU::Table *currentTable = nullptr;
    profile_t init_apply(const IR::Node *) override;
    bool preorder(IR::Expression *) override { return false; }
    void postorder(IR::MAU::Table *) override;
    void verify_format(const IXBar::Use *);
    class GetCurrentUse;
    // Array of Map of Stage -> Input Xbar
    // Tofino 1/2 only uses ixbar[0] for both ingress and egress.
    std::map<int, std::unique_ptr<IXBar>> ixbar[2];

 public:
    explicit IXBarVerify(const PhvInfo &phv) : phv(phv) {}
};

#endif /* BF_P4C_MAU_IXBAR_REALIGN_H_ */
