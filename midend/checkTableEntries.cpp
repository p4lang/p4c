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

static int get_width_bits(const P4::IR::Type *type) {
    if (!type) return 0;
    if (auto *tb = type->to<P4::IR::Type_Bits>()) return tb->width_bits();
    if (auto *se = type->to<P4::IR::Type_SerEnum>()) return get_width_bits(se->type);
    if (auto *td = type->to<P4::IR::Type_Typedef>()) return get_width_bits(td->type);
    if (auto *nt = type->to<P4::IR::Type_Newtype>()) return get_width_bits(nt->type);
    if (type->is<P4::IR::Type_Boolean>()) return 1;
    return 0;
}

/* Given an expression for an entry key, extract the mask and test value.
 * A key value will match this entry iff (key & mask == val) */
bool P4::CheckTableEntries::get_mask_val(const IR::Expression *e, int width, big_int &mask,
                                         big_int &val) {
    while (auto *c = e->to<IR::Cast>()) e = c->expr;
    if (auto *m = e->to<IR::Mask>()) {
        auto *l = m->left;
        while (auto *c = l->to<IR::Cast>()) l = c->expr;
        auto *r = m->right;
        while (auto *c = r->to<IR::Cast>()) r = c->expr;
        auto *lc = l->to<IR::Constant>();
        auto *rc = r->to<IR::Constant>();
        if (lc && rc) {
            mask = rc->value;
            val = lc->value;
            if (width > 0) {
                mask &= IR::Constant::GetMask(width).value;
                val &= mask;
            }
            return true;
        }
    } else if (auto *k = e->to<IR::Constant>()) {
        val = k->value;
        int w = width > 0 ? width : get_width_bits(k->type);
        if (w > 0) {
            mask = IR::Constant::GetMask(w).value;
            val &= mask;
        } else {
            mask = -1;
        }
        return true;
    } else if (auto *bl = e->to<IR::BoolLiteral>()) {
        mask = 1;
        val = bl->value ? 1 : 0;
        return true;
    } else if (e->is<IR::DefaultExpression>()) {
        mask = val = 0;
        return true;
    }
    return false;
}

/* Check if two entry key components match the exact same value/range. */
bool P4::CheckTableEntries::keys_equal(const IR::Expression *k1, const IR::Expression *k2,
                                       int width) {
    while (auto *c = k1->to<IR::Cast>()) k1 = c->expr;
    while (auto *c = k2->to<IR::Cast>()) k2 = c->expr;

    if (auto *r1 = k1->to<IR::Range>()) {
        if (auto *r2 = k2->to<IR::Range>()) {
            auto *l1 = r1->left;
            while (auto *c = l1->to<IR::Cast>()) l1 = c->expr;
            auto *right1 = r1->right;
            while (auto *c = right1->to<IR::Cast>()) right1 = c->expr;
            auto *l2 = r2->left;
            while (auto *c = l2->to<IR::Cast>()) l2 = c->expr;
            auto *right2 = r2->right;
            while (auto *c = right2->to<IR::Cast>()) right2 = c->expr;

            auto *c_l1 = l1->to<IR::Constant>();
            auto *c_r1 = right1->to<IR::Constant>();
            auto *c_l2 = l2->to<IR::Constant>();
            auto *c_r2 = right2->to<IR::Constant>();
            if (c_l1 && c_r1 && c_l2 && c_r2)
                return c_l1->value == c_l2->value && c_r1->value == c_r2->value;
        }
        return k1->equiv(*k2);
    }

    big_int m1, v1, m2, v2;
    if (get_mask_val(k1, width, m1, v1) && get_mask_val(k2, width, m2, v2)) {
        return m1 == m2 && (v1 & m1) == (v2 & m2);
    }

    return k1->equiv(*k2);
}

/* Check if the first ternary entry key covers the second one, meaning every
 * value matched by the second key is also matched by the first key. */
bool P4::CheckTableEntries::ternary_covers(const IR::Expression *k1, const IR::Expression *k2,
                                           int width) {
    big_int k1_mask, k1_val, k2_mask, k2_val;
    if (!get_mask_val(k1, width, k1_mask, k1_val) || !get_mask_val(k2, width, k2_mask, k2_val))
        return false;
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
            bool is_duplicate = true;
            bool is_covered = true;

            for (unsigned i = 0; i < entry->keys->size(); ++i) {
                auto *k1 = prev->components[i];
                auto *k2 = entry->keys->components[i];
                auto *key_el = key->keyElements[i];
                int width = get_width_bits(key_el->expression->type);

                if (keys_equal(k1, k2, width)) {
                    continue;
                }

                is_duplicate = false;
                if (ternary_keys[i] && ternary_covers(k1, k2, width)) {
                    continue;
                }

                is_covered = false;
                break;
            }

            if (is_duplicate) {
                if (genError)
                    error(ErrorType::ERR_TABLE_KEYS, "%1%%2%Duplicate entry keys",
                          entry->keys->srcInfo, prev->srcInfo);
                else
                    warning(ErrorType::WARN_TABLE_KEYS, "%1%%2%Duplicate entry keys",
                            entry->keys->srcInfo, prev->srcInfo);
                break;
            } else if (is_covered) {
                warning(ErrorType::WARN_TABLE_KEYS, "%1%%2%Ternary entry covered by previous entry",
                        entry->keys->srcInfo, prev->srcInfo);
                break;
            }
        }
        prev_keys.push_back(entry->keys);
    }
    return false;
}
