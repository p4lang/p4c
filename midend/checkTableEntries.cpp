/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "checkTableEntries.h"

using namespace P4::literals;

/* Given an expression for a entry key , extract the mask and test value.
 * A key value will match this entry iff (key & mask == val) */
void P4::CheckTableEntries::get_mask_val(const IR::Expression *e, big_int &mask, big_int &val) {
    if (auto *m = e->to<IR::Mask>()) {
        auto *l = m->left->to<IR::Constant>();
        auto *r = m->right->to<IR::Constant>();
        if (l && r) {
            mask = r->value;
            val = l->value;
        } else
            error(ErrorType::ERR_UNEXPECTED, "%1% Unexpected entry key expression", e);
    } else if (auto *k = e->to<IR::Constant>()) {
        val = k->value;
        if (auto w = k->type->width_bits(); w > 0)
            mask = IR::Constant::GetMask(w).value;
        else
            mask = -1;
    } else if (auto *bl = e->to<IR::BoolLiteral>()) {
        mask = 1;
        val = bl->value ? 1 : 0;
    } else if (e->is<IR::DefaultExpression>()) {
        mask = val = 0;
    } else {
        error(ErrorType::ERR_UNEXPECTED, "%1% Unexpected entry key expression", e);
    }
}

/* Check two ternary entry keys to see if first one "covers" the second one -- that is, the
 * the first one will always match if the second one does.  That is,  ∀v: v∈k2 -> v∈k1
 */
bool P4::CheckTableEntries::ternary_covers(const IR::Expression *k1, const IR::Expression *k2) {
    big_int k1_mask, k1_val, k2_mask, k2_val;
    get_mask_val(k1, k1_mask, k1_val);
    get_mask_val(k2, k2_mask, k2_val);
    if ((k1_mask & k2_mask) != k1_mask) return false;
    return (k1_val & k1_mask) == (k2_val & k1_mask);
}

bool P4::CheckTableEntries::preorder(const IR::P4Table *tbl) {
    auto *entries = tbl->getEntries();
    if (!entries || entries->entries.empty()) return false;
    auto *key = tbl->getKey();
    BUG_CHECK(key, "%1% table has entries and no key", tbl);
    std::vector<bool> ternary_keys;
    for (auto *key_el : key->keyElements) {
        cstring matchKind = key_el->matchType->path->name.name;
        ternary_keys.push_back(matchKind == "ternary"_cs || matchKind == "optional"_cs);
    }

    // FIXME -- need a way of hashing or ordering ListExpressions so we can use a map
    // however, need to leave ternary key fields out of that hash/order.  For now we just
    // look at all of them -- O(n^2) in the number of entries
    std::vector<const IR::ListExpression *> prev_keys;

    for (auto *entry : entries->entries) {
        BUG_CHECK(entry->keys->size() == ternary_keys.size(), "%1% key size mismatch", entry);

        for (auto *prev : prev_keys) {
            bool no_match = false, ternary_match = false;
            for (unsigned i = 0; i < entry->keys->size(); ++i) {
                if (entry->keys->components[i]->equiv(*prev->components[i])) {
                    // matches
                } else if (ternary_keys[i]) {
                    ternary_match = true;
                    if (!ternary_covers(prev->components[i], entry->keys->components[i])) {
                        no_match = true;
                        break;
                    }
                } else {
                    no_match = true;
                    break;
                }
            }
            if (no_match) continue;
            if (ternary_match)
                warning(ErrorType::WARN_TABLE_KEYS, "%1%%2%Ternary entry covered by previous entry",
                        entry->keys->srcInfo, prev->srcInfo);
            else if (genError)
                error(ErrorType::ERR_TABLE_KEYS, "%1%%2%Duplicate entry keys", entry->keys->srcInfo,
                      prev->srcInfo);
            else
                warning(ErrorType::WARN_TABLE_KEYS, "%1%%2%Duplicate entry keys",
                        entry->keys->srcInfo, prev->srcInfo);
            // don't bother with more than one error/warning per entry
            break;
        }
        prev_keys.push_back(entry->keys);
    }
    return false;
}
