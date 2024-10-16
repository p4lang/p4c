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

#include "ir/ir.h"
#include "field_use.h"

bool FieldUse::preorder(const IR::MAU::Table *t) {
    table_use[t->name];
    return true;
}

void FieldUse::access_field(cstring name) {
    int idx;
    if (!field_index.count(name)) {
        idx = field_index[name] = field_names.size();
        field_names.push_back(name);
    } else {
        idx = field_index[name]; }
    if (auto table = findContext<IR::MAU::Table>()) {
        if (isWrite())
            table_use[table->name].writes[idx] = 1;
        else
            table_use[table->name].reads[idx] = 1;
    } else {
        assert(0); }
}

bool FieldUse::preorder(const IR::Member *f) {
    auto* field = phv.field(f);
    if (!field) {
        access_field(f->toString());
        return false;
    }
    const PHV::Field* alias = phv.getAliasDestination(field);
    // If this field is an alias source, then ensure that we use the alias destination to record the
    // access.
    if (alias == nullptr)
        access_field(f->toString());
    else
        access_field(alias->name);
    return false;
}

bool FieldUse::preorder(const IR::HeaderStackItemRef *f) {
    access_field(f->toString());
    return false;
}

bool FieldUse::preorder(const IR::TempVar *f) {
    access_field(f->toString());
    return false;
}

bitvec FieldUse::tables_modify(const IR::MAU::Table *t) const {
    bitvec rv;
    rv |= table_use.at(t->name).writes;
    for (auto &n : t->next)
        if (n.second)
            rv |= tables_modify(n.second);
    return rv;
}

bitvec FieldUse::tables_access(const IR::MAU::Table *t) const {
    bitvec rv;
    rv |= table_use.at(t->name).reads;
    rv |= table_use.at(t->name).writes;
    for (auto &n : t->next)
        if (n.second)
            rv |= tables_access(n.second);
    return rv;
}


bitvec FieldUse::tables_modify(const IR::MAU::TableSeq *g) const {
    bitvec rv;
    for (auto t : g->tables)
        rv |= tables_modify(t);
    return rv;
}

bitvec FieldUse::tables_access(const IR::MAU::TableSeq *g) const {
    bitvec rv;
    for (auto t : g->tables)
        rv |= tables_access(t);
    return rv;
}

std::ostream &operator<<(std::ostream &out, const FieldUse &f) {
    for (auto &t : f.table_use) {
        out << std::setw(20) << t.first << " R:";
        for (auto i : t.second.reads) out << ' ' << f.field_names[i];
        out << std::endl;
        out << std::setw(20) << " " << " W:";
        for (auto i : t.second.writes) out << ' ' << f.field_names[i];
        out << std::endl; }
    return out;
}
