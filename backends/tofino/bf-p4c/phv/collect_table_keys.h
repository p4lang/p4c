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
    const TableProp& prop(const IR::MAU::Table *tbl) const { return table_props.at(tbl); }
    const ordered_map<const IR::MAU::Table *, TableProp> &props() const { return table_props; }
};

}  // namespace PHV

#endif /* BF_P4C_PHV_COLLECT_TABLE_KEYS_H_ */
