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

#include "bf-p4c/mau/handle_assign.h"

#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "lib/safe_vector.h"

bool AssignActionHandle::ActionProfileImposedConstraints::preorder(const IR::MAU::ActionData *ad) {
    auto tbl = findContext<IR::MAU::Table>();

    std::set<cstring> actions;
    for (auto act : Values(tbl->actions)) {
        actions.insert(act->name);
    }

    if (profile_actions.count(ad) == 0) {
        profile_actions[ad] = actions;
        return false;
    }

    auto curr_actions = profile_actions.at(ad);
    std::set<cstring> intersect;
    std::set_intersection(actions.begin(), actions.end(), curr_actions.begin(), curr_actions.end(),
                          std::inserter(intersect, intersect.end()));
    std::set<cstring> difference;
    if (intersect.size() < actions.size()) {
        std::set_difference(actions.begin(), actions.end(), intersect.begin(), intersect.end(),
                            std::inserter(difference, difference.end()));
    } else if (intersect.size() < curr_actions.size()) {
        std::set_difference(curr_actions.begin(), curr_actions.end(), intersect.begin(),
                            intersect.end(), std::inserter(difference, difference.end()));
    }

    if (!difference.empty()) {
        std::string sep = "";
        std::string non_shared_actions = "";
        for (auto entry : difference) {
            non_shared_actions += sep + entry;
            sep = ", ";
        }
        error(
            "%s: Currently in p4c, any table using an action profile is required to use "
            "the same actions, and the following actions don't appear in all table using "
            "the action profile %s : %s",
            ad->srcInfo, ad->name, non_shared_actions);
    }
    return false;
}

Visitor::profile_t AssignActionHandle::DetermineHandle::init_apply(const IR::Node *root) {
    profile_t rv = MauInspector::init_apply(root);
    handle_position = 0;
    profile_assignments.clear();
    self.handle_assignments.clear();
    return rv;
}

/**
 * Assign the action handle
 */
bool AssignActionHandle::DetermineHandle::preorder(const IR::MAU::Action *act) {
    auto tbl = findContext<IR::MAU::Table>();
    const IR::MAU::ActionData *ad = nullptr;
    for (auto ba : tbl->attached) {
        ad = ba->attached->to<IR::MAU::ActionData>();
        if (ad != nullptr) break;
    }

    if (ad == nullptr) {
        self.handle_assignments[act] = next_handle();
        return false;
    }

    auto &profile_map = profile_assignments[ad];
    if (profile_map.count(act->name)) {
        self.handle_assignments[act] = profile_map.at(act->name);
    } else {
        unsigned handle = next_handle();
        profile_map[act->name] = handle;
        self.handle_assignments[act] = handle;
    }
    return false;
}

/**
 * Modify the action with the action handle
 */
bool AssignActionHandle::AssignHandle::preorder(IR::MAU::Action *act) {
    auto orig_act = getOriginal()->to<IR::MAU::Action>();
    act->handle = self.handle_assignments.at(orig_act);
    return false;
}

Visitor::profile_t AssignActionHandle::ValidateSelectors::init_apply(const IR::Node *root) {
    profile_t rv = PassManager::init_apply(root);
    selector_keys.clear();
    initial_table.clear();
    table_to_selector.clear();
    return rv;
}

bool AssignActionHandle::ValidateSelectors::ValidateKey::preorder(const IR::MAU::Selector *sel) {
    if (findContext<IR::MAU::StatefulAlu>()) return false;
    auto tbl = findContext<IR::MAU::Table>();
    if (!tbl) return false;
    BUG_CHECK(tbl != nullptr, "No associated table found for Selector - %1%", sel);

    safe_vector<const IR::Expression *> sel_key_vec;
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_selection()) continue;
        sel_key_vec.push_back(ixbar_read->expr);
    }

    if (sel_key_vec.empty()) {
        error("%s: On Table %s, the Selector %s is provided no keys", sel->srcInfo, tbl->name,
              sel->name);
        return false;
    }

    auto sel_entry = self.selector_keys.find(sel);
    if (sel_entry != self.selector_keys.end() &&
        sel->max_pool_size > StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE) {
        /**
         * Due to the register rams.match.merge.mau_meter_alu_to_logical_map being an OXBar,
         * one can only assign a single logical table to a wide hash mod.  Thus, a selector
         * that requires a hash mod cannot be shared
         */
        error(
            "%s: The selector %s cannot be shared between tables %s and %s, because "
            "it requires a max pool size of %d.  In order to share a selector on Barefoot "
            "HW, the max pool size must be %d",
            sel->srcInfo, sel->name, tbl->name, self.initial_table.at(sel), sel->max_pool_size,
            StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE);
    }
    auto &ixbSpec = Device::ixbarSpec();
    le_bitrange hash_bits = {
        0, (sel->mode == IR::MAU::SelectorMode::RESILIENT ? ixbSpec.resilientModeHashBits()
                                                          : ixbSpec.fairModeHashBits()) -
               1};

    P4HashFunction *sel_func = new P4HashFunction();
    sel_func->inputs = sel_key_vec;
    sel_func->hash_bits = hash_bits;
    sel_func->algorithm = sel->algorithm;
    verifySymmetricHashPairs(self.phv, sel_func->inputs, sel->annotations, tbl->gress,
                             sel->algorithm, &sel_func->symmetrically_hashed_inputs);

    self.table_to_selector[tbl] = sel;

    if (sel_entry == self.selector_keys.end()) {
        self.selector_keys[sel] = sel_func;
        self.initial_table[sel] = tbl;
    } else {
        auto sel_func_comp = self.selector_keys.at(sel);
        if (!sel_func->equiv(sel_func_comp)) {
            error(
                "%s: The key for selector %s on table %s does not match the key for the "
                "selector on table %s.  Barefoot requires the selector key to be identical "
                "per selector",
                sel->srcInfo, sel->name, tbl->name, self.initial_table.at(sel));
        }
    }
    return false;
}

bool AssignActionHandle::ValidateSelectors::SetSymmetricSelectorKeys::preorder(
    IR::MAU::Table *tbl) {
    auto orig_tbl = getOriginal()->to<IR::MAU::Table>();
    auto sel_itr = self.table_to_selector.find(orig_tbl);
    if (sel_itr == self.table_to_selector.end()) return true;
    auto hf = self.selector_keys.at(sel_itr->second);
    tbl->sel_symmetric_keys = hf->symmetrically_hashed_inputs;
    return true;
}

Visitor::profile_t AssignActionHandle::GuaranteeUniqueHandle::init_apply(const IR::Node *root) {
    auto rv = MauInspector::init_apply(root);
    unique_handle.clear();
    action_profiles.clear();
    return rv;
}

/**
 * Ensures that an action handle is unique within a P4 program
 *
 */
bool AssignActionHandle::GuaranteeUniqueHandle::preorder(const IR::MAU::Action *act) {
    visitOnce();
    auto tbl = findContext<IR::MAU::Table>();
    const IR::MAU::ActionData *ad = nullptr;
    for (auto ba : tbl->attached) {
        ad = ba->attached->to<IR::MAU::ActionData>();
        if (ad != nullptr) break;
    }

    if (ad != nullptr) {
        if (action_profiles.count(ad)) {
            auto pos = unique_handle.find(act->handle);
            if (pos != unique_handle.end() && pos->second->name != act->name) {
                BUG("On action profile %s action %s has a bad handle collision", ad->name,
                    act->name);
            }
            return false;
        } else {
            action_profiles.insert(ad);
        }
    }

    auto pos = unique_handle.find(act->handle);
    if (pos != unique_handle.end()) {
        BUG("Actions %s and %s were assigned the same handle", pos->second->name, act->name);
    } else {
        unique_handle[act->handle] = act;
    }
    return false;
}
