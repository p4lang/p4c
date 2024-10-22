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

#ifndef BF_P4C_MAU_TABLE_CONTROL_DEPS_H_
#define BF_P4C_MAU_TABLE_CONTROL_DEPS_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "lib/ordered_map.h"

using namespace P4;

/** Find control dependencies between tables in the presence of multiply applied tables
 * Table B is control dependent on table A if and only if A appears on all IR paths from
 * the root IR::BFN::Pipe to table B.  So we find the set of all tables on each path and
 * take the intersection of those sets
 * We also calculate the number of distinct dependent paths from each table here.
 */
class TableControlDeps : public MauTableInspector {
    struct info_t {  // info per table
        ordered_set<const IR::MAU::Table *> parents = {};
        int dependent_paths = 0;
    };
    ordered_map<const IR::MAU::Table *, info_t> info;

    profile_t init_apply(const IR::Node *root) override {
        info.clear();
        return MauTableInspector::init_apply(root);
    }
    ordered_set<const IR::MAU::Table *> parents();
    void postorder(const IR::MAU::Table *tbl) override;
    void revisit(const IR::MAU::Table *tbl) override;
    void postorder(const IR::MAU::TableSeq *) override { visitAgain(); }

 public:
    bool operator()(const IR::MAU::Table *a, const IR::MAU::Table *b) const;
    int paths(const IR::MAU::Table *) const;
};

#endif /* BF_P4C_MAU_TABLE_CONTROL_DEPS_H_ */
