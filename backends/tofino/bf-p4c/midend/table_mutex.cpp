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

#include "table_mutex.h"

#include "frontends/p4/methodInstance.h"
#include "lib/small_set.h"
#include "lib/symbitmatrix.h"

struct TableMutex::Shared {
    std::map<cstring, int> tableByName;
    std::vector<cstring> tableById;
    std::map<cstring, SmallSet<int>> tablesForAction;
    SymBitMatrix mutex;
    SymBitMatrix not_mutex;  // temp within one control
    bitvec seen_in_control;
};

TableMutex::TableMutex() { data = new Shared; }

void TableMutex::flow_merge(Visitor &v) {
    auto &other = dynamic_cast<TableMutex &>(v);
    BUG_CHECK(other.data == data, "mismatched clone");
    seen_tables |= other.seen_tables;
}

void TableMutex::flow_copy(ControlFlowVisitor &v) {
    auto &other = dynamic_cast<TableMutex &>(v);
    BUG_CHECK(other.data == data, "mismatched clone");
    seen_tables = other.seen_tables;
}

bool TableMutex::preorder(const IR::P4Control *c) {
    Log::TempIndent indent;
    LOG2("Find table mutex for control " << c->name);
    size_t first_table = data->tableById.size();
    for (auto *decl : c->controlLocals) {
        if (auto *tbl = decl->to<IR::P4Table>()) {
            int tbl_id = data->tableById.size();
            if (size_t(tbl_id) == first_table && data->tableByName.count(tbl->name.name)) {
                // FIXME --This can happen when a v1model program uses the same control for
                // ingress and egress, and TNA coversion duplicates it after UniqueNames
                // has run.  This makes no sense in real programs, but we use it gtests
                // Since egress is identical to ingress, we can just skip it.
                // Arguably we should rerun UniqueNames if TNA conversion needs to
                // duplicate a control like this.
                LOG2("skipping " << c->name << " as it is a duplicate of ingress");
                return false;
            }
            BUG_CHECK(!data->tableByName.count(tbl->name.name), "duplicate table name");
            LOG3("table " << tbl->name << " is #" << tbl_id);
            data->tableByName[tbl->name.name] = tbl_id;
            data->tableById.push_back(tbl->name.name);
            auto *actions = tbl->getActionList();
            BUG_CHECK(actions, "%s has no actions?", tbl);
            for (auto *act : actions->actionList)
                data->tablesForAction[act->getName().name].insert(tbl_id);
        }
    }
    if (first_table == data->tableById.size()) {
        LOG2("no tables in " << c->name);
        return false;
    }
    data->not_mutex.clear();
    data->seen_in_control.clear();
    visit(c->body, "body");
    for (auto i = first_table; i < data->tableById.size(); ++i) {
        if (!data->seen_in_control)
            LOG2("warning -- " << data->tableById.at(i) << " not seen in control");
        data->mutex[i] |= data->seen_in_control - data->not_mutex[i];
    }
    return false;
}

bool TableMutex::preorder(const IR::MethodCallExpression *mce) {
    auto *mi = P4::MethodInstance::resolve(mce, this);
    if (!mi) return false;
    if (auto *apply = mi->to<P4::ApplyMethod>()) {
        auto *tbl = apply->object->to<IR::P4Table>();
        BUG_CHECK(tbl, "Apply on non-table %s", apply->object);
        int id = data->tableByName.at(tbl->name.name);
        seen_tables[id] = 1;
        data->seen_in_control[id] = 1;
        data->not_mutex[id] |= seen_tables;
    }
    return false;
}

void TableMutex::end_apply() {
    LOG4("TableMutex result:" << Log::indent << Log::endl << *this << Log::unindent);
}

bool TableMutex::operator()(const IR::P4Table *a, const IR::P4Table *b) {
    int a_id = data->tableByName.at(a->name.name);
    int b_id = data->tableByName.at(b->name.name);
    return data->mutex(a_id, b_id);
}

bool TableMutex::operator()(const IR::P4Action *a, const IR::P4Action *b) {
    for (int a_id : data->tablesForAction.at(a->name.name)) {
        for (int b_id : data->tablesForAction.at(b->name.name)) {
            if (a_id == b_id) continue;
            if (!data->mutex(a_id, b_id)) return false;
        }
    }
    return true;
}

bool TableMutex::operator()(const IR::Declaration *d1, const IR::Declaration *d2) {
    if (auto *t1 = d1->to<IR::P4Table>()) {
        auto *t2 = d2->to<IR::P4Table>();
        BUG_CHECK(t2, "Can't compare mutex of table %1% and non-table %2%", d1, d2);
        return operator()(t1, t2);
    }
    if (auto *a1 = d1->to<IR::P4Action>()) {
        auto *a2 = d2->to<IR::P4Action>();
        BUG_CHECK(a2, "Can't compare mutex of action %1% and non-action %2%", d1, d2);
        return operator()(a1, a2);
    }
    BUG("Can't compare mutex of non table/actions %1% and %2%", d1, d2);
    return false;
}

std::ostream &operator<<(std::ostream &out, const TableMutex &mutex) {
    for (size_t i = 0; i < mutex.data->tableById.size(); ++i) {
        if (i > 0) out << Log::endl;
        for (size_t j = 0; j < i; ++j) out << (mutex.data->mutex(i, j) ? '1' : '0');
        out << "+-- " << mutex.data->tableById.at(i);
    }
    return out;
}
