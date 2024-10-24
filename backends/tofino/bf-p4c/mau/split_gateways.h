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

#ifndef BF_P4C_MAU_SPLIT_GATEWAYS_H_
#define BF_P4C_MAU_SPLIT_GATEWAYS_H_

#include "field_use.h"
#include "gateway.h"
#include "mau_visitor.h"

using namespace P4;

class SpreadGatewayAcrossSeq : public MauTransform, public Backtrack {
    FieldUse uses;
    bool do_splitting = false;
    struct enable : public Backtrack::trigger {
        DECLARE_TYPEINFO(enable);
    };
    bool backtrack(trigger &trig) override {
        if (!do_splitting && trig.is<enable>()) {
            do_splitting = true;
            return true;
        }
        return false;
    }
    Visitor::profile_t init_apply(const IR::Node *) override;
    const IR::Node *postorder(IR::MAU::Table *) override;

 public:
    explicit SpreadGatewayAcrossSeq(const PhvInfo &p) : uses(p) {}
};

class SplitComplexGateways : public Transform {
    const PhvInfo &phv;
    const IR::MAU::Table *preorder(IR::MAU::Table *tbl) override;

 public:
    explicit SplitComplexGateways(const PhvInfo &phv) : phv(phv) {}
};

#endif /* BF_P4C_MAU_SPLIT_GATEWAYS_H_ */
