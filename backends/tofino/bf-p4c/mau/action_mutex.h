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

#ifndef BF_P4C_MAU_ACTION_MUTEX_H_
#define BF_P4C_MAU_ACTION_MUTEX_H_

#include <map>

#include "bf-p4c/mau/mau_visitor.h"
#include "lib/symbitmatrix.h"

using namespace P4;

class ActionMutuallyExclusive : public MauInspector {
    // map action to ids
    std::map<const IR::MAU::Action *, int> action_ids;
    // map table to all actions that will be executed after applying it.
    std::map<const IR::MAU::Table *, bitvec> action_succ;
    // action mutex matrix
    SymBitMatrix not_mutex;
    bool preorder(const IR::MAU::Table *t) override {
        for (const auto *act : Values(t->actions)) {
            if (!action_ids.count(act)) action_ids.emplace(act, action_ids.size());
            name_actions[t->externalName() + "." + act->name] = act;
        }
        return true;
    }
    void postorder(const IR::MAU::Table *tbl) override;
    void postorder(const IR::MAU::TableSeq *seq) override;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = MauInspector::init_apply(root);
        action_ids.clear();
        action_succ.clear();
        not_mutex.clear();
        return rv;
    }

 public:
    bool operator()(const IR::MAU::Action *a, const IR::MAU::Action *b) const {
        return !not_mutex(action_ids.at(a), action_ids.at(b));
    }

    // For unit-tests
    const std::map<const IR::MAU::Action *, int> &actions() const { return action_ids; }
    // Maping action names to pointers
    std::map<cstring, const IR::MAU::Action *> name_actions;
};

#endif /* BF_P4C_MAU_ACTION_MUTEX_H_ */
