/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stage.h"
#include "tables.h"

static int find_in_ixbar(Table *table, std::vector<Phv::Ref> &match) {
    // It would seem like it would be possible to simplify this code by refactoring it
    // to use one loop calling Table::find_on_ixbar (which does much of what this does),r
    // but it is important to prefer a group defined in this table to one defined in other
    // tables, which the two loops does.  Could perhaps have a variant of find_on_ixbar that
    // return *all* groups where the Phv::Ref is present (in priority order), so we could
    // do the intersection (preserving priority order) rather than this repeated looping?
    int max_i = -1;
    LOG3("find_in_ixbar " << match);
    for (unsigned group = 0; group < EXACT_XBAR_GROUPS; group++) {
        LOG3(" looking in table in group " << group);
        bool ok = true;
        for (auto &r : match) {
            LOG3("  looking for " << r);
            for (auto &ixb : table->input_xbar) {
                if (!ixb->find_exact(*r, group)) {
                    LOG3("   -- not found");
                    ok = false;
                    break;
                }
            }
        }
        if (ok) {
            LOG3(" success");
            return group;
        }
    }
    for (unsigned group = 0; group < EXACT_XBAR_GROUPS; group++) {
        LOG3(" looking in group " << group);
        bool ok = true;
        for (auto &r : match) {
            LOG3("  looking for " << r);
            bool found = false;
            InputXbar::Group ixbar_group(InputXbar::Group::EXACT, group);
            for (auto *in : table->stage->ixbar_use[ixbar_group]) {
                if (in->find_exact(*r, group)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                LOG3("   -- not found");
                if (&r - &match[0] > max_i) max_i = &r - &match[0];
                ok = false;
                break;
            }
        }
        if (ok) {
            LOG3(" success");
            return group;
        }
    }
    if (max_i > 0)
        error(match[max_i].lineno, "%s: Can't find %s and %s in same input xbar group",
              table->name(), match[max_i].name(), match[0].name());
    else
        error(match[0].lineno, "%s: Can't find %s in any input xbar group", table->name(),
              match[0].name());
    return -1;
}

void SRamMatchTable::setup_word_ixbar_group(Target::Tofino) {
    word_ixbar_group.resize(match_in_word.size());
    unsigned i = 0;
    for (auto &match : match_in_word) {
        std::vector<Phv::Ref> phv_ref_match;
        for (auto *source : match) {
            auto phv_ref = dynamic_cast<Phv::Ref *>(source);
            BUG_CHECK(phv_ref);
            BUG_CHECK(*phv_ref);
            phv_ref_match.push_back(*phv_ref);
        }
        word_ixbar_group[i++] = phv_ref_match.empty() ? -1 : find_in_ixbar(this, phv_ref_match);
    }
}
