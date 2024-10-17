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

#ifndef BF_P4C_MAU_FIELD_USE_H_
#define BF_P4C_MAU_FIELD_USE_H_

#include <iostream>
#include "mau_visitor.h"
#include "lib/bitvec.h"
#include "lib/safe_vector.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

class FieldUse : public MauInspector, TofinoWriteContext {
    const PhvInfo&              phv;
    safe_vector<cstring>        field_names;
    std::map<cstring, int>      field_index;
    struct rw_t { bitvec reads, writes; };
    std::map<cstring, rw_t>     table_use;
    void access_field(cstring field);
    bool preorder(const IR::MAU::Table *t) override;
    bool preorder(const IR::Member *f) override;
    bool preorder(const IR::HeaderStackItemRef *f) override;
    bool preorder(const IR::TempVar *t) override;
    friend std::ostream &operator<<(std::ostream &, const FieldUse &);
    profile_t init_apply(const IR::Node *root) override {
        static int counter;
        auto rv = MauInspector::init_apply(root);
        LOG1("Field Use call #" << ++counter);
        return rv; }
    void end_apply() override { LOG3(*this); }

 public:
    explicit FieldUse(const PhvInfo& p) : phv(p) { visitDagOnce = false; }
    bitvec tables_modify(const IR::MAU::TableSeq *t) const;
    bitvec tables_access(const IR::MAU::TableSeq *t) const;
    bitvec tables_modify(const IR::MAU::Table *t) const;
    bitvec tables_access(const IR::MAU::Table *t) const;
    bitvec table_reads(const IR::MAU::Table *t) const {
        return table_use.at(t->name).reads; }
    bitvec table_writes(const IR::MAU::Table *t) const {
        return table_use.at(t->name).writes; }
    void cloning_table(cstring name, cstring newname) {
        assert(table_use.count(name) && !table_use.count(newname));
        table_use[newname] = table_use[name]; }
};


#endif /* BF_P4C_MAU_FIELD_USE_H_ */
