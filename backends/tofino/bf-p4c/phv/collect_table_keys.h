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

#ifndef BF_P4C_PHV_COLLECT_TABLE_KEYS_H_
#define BF_P4C_PHV_COLLECT_TABLE_KEYS_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"

namespace PHV {

class CollectTableKeys : public MauInspector {
 public:
    struct TableProp {
        ordered_set<PHV::FieldSlice> keys;
        int n_entries = -1;
        bool is_tcam = false;
        bool is_range = false;
    };

 private:
    const PhvInfo &phv;
    ordered_map<const IR::MAU::Table *, TableProp> table_props;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = MauInspector::init_apply(root);
        table_props.clear();
        return rv;
    }
    void end_apply() override;
    bool preorder(const IR::MAU::Table *tbl) override;
    int get_n_entries(const IR::MAU::Table *tbl) const;

 public:
    explicit CollectTableKeys(const PhvInfo &p) : phv(p) {}
    const TableProp &prop(const IR::MAU::Table *tbl) const { return table_props.at(tbl); }
    const ordered_map<const IR::MAU::Table *, TableProp> &props() const { return table_props; }
};

}  // namespace PHV

#endif /* BF_P4C_PHV_COLLECT_TABLE_KEYS_H_ */
