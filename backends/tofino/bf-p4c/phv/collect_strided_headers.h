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

#ifndef BF_P4C_PHV_COLLECT_STRIDED_HEADERS_H_
#define BF_P4C_PHV_COLLECT_STRIDED_HEADERS_H_

#include <map>

#include "bf-p4c/phv/phv_fields.h"
#include "lib/cstring.h"

/// Collects header stacks that require strided allocation (in a parser loop).
struct CollectStridedHeaders : public Inspector {
    PhvInfo &phv;

    std::map<cstring, std::vector<ordered_set<const PHV::Field *>>> stride_groups;

    explicit CollectStridedHeaders(PhvInfo &p) : phv(p) {}

    bool preorder(const IR::HeaderStack *hs) {
        auto state = findContext<IR::BFN::ParserState>();

        if (state && state->stride) {
            LOG4(state->name << " needs strided allocation (stride size = " << hs->size << ")");

            for (auto f : hs->type->fields) {
                ordered_set<const PHV::Field *> stride_group;

                for (int i = 0; i < hs->size; i++) {
                    cstring name = hs->name + "[" + cstring::to_cstring(i) + "]." + f->name;
                    auto field = phv.field(name);
                    stride_group.push_back(field);
                }

                stride_groups[hs->name].push_back(stride_group);
            }
        }

        return false;
    }

    const ordered_set<const PHV::Field *> *get_strided_group(const PHV::Field *f) const {
        for (auto &[_, groups] : stride_groups) {
            for (auto &group : groups) {
                if (group.count(f)) return &group;
            }
        }

        return nullptr;
    }

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        stride_groups.clear();
        return rv;
    }
};

#endif /* BF_P4C_PHV_COLLECT_STRIDED_HEADERS_H_ */
