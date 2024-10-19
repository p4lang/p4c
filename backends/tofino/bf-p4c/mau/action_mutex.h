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
