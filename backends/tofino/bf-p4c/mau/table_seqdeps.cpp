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

#include "table_seqdeps.h"

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/ltbitmatrix.h"

Visitor::profile_t TableFindSeqDependencies::init_apply(const IR::Node *root) {
    auto rv = MauModifier::init_apply(root);
    root->apply(uses);
    LOG2(uses);
    return rv;
}

static bool ignore_dep(const IR::MAU::Table *t1, const IR::MAU::Table *t2) {
    std::vector<IR::ID> pragmas;
    if (t1->getAnnotation("ignore_table_dependency"_cs, pragmas)) {
        for (auto name : pragmas) {
            if (t2->externalName().endsWith(name.string())) return true;
        }
    }
    pragmas.clear();
    if (t2->getAnnotation("ignore_table_dependency"_cs, pragmas)) {
        for (auto name : pragmas) {
            if (t1->externalName().endsWith(name.string())) return true;
        }
    }
    return false;
}

void TableFindSeqDependencies::postorder(IR::MAU::TableSeq *seq) {
    if (seq->tables.size() <= 1) return;
    int size = seq->tables.size();
    seq->deps.clear();
    for (int i = 0; i < size; i++) {
        bitvec writes = uses.tables_modify(seq->tables[i]);
        bitvec access = uses.tables_access(seq->tables[i]);
        bool earlyExit = seq->tables[i]->has_exit_recursive();
        for (int j = i + 1; j < size; j++) {
            if (ignore_dep(seq->tables[i], seq->tables[j])) continue;
            if (earlyExit || seq->tables[j]->has_exit_recursive() ||
                (writes & uses.tables_access(seq->tables[j])) ||
                (access & uses.tables_modify(seq->tables[j])))
                seq->deps(j, i) = true;
        }
    }
    for (int j = 1; j < size; j++)
        for (int i = j - 1; i > 0; i--)
            if (seq->deps(j, i)) seq->deps[j] |= seq->deps[i];
}
