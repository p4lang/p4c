// Copyright 2019 VMware, Inc.
// SPDX-FileCopyrightText: 2019 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "removeMiss.h"

#include "frontends/p4/tableApply.h"

namespace P4 {

const IR::Node *DoRemoveMiss::preorder(IR::Member *expression) {
    if (!TableApplySolver::isMiss(expression, this, typeMap)) return expression;
    auto hit = new IR::Member(expression->expr, IR::Type_Table::hit);
    return new IR::LNot(hit);
}

const IR::Node *DoRemoveMiss::preorder(IR::IfStatement *statement) {
    if (!TableApplySolver::isMiss(statement->condition, this, typeMap)) return statement;
    auto mem = statement->condition->checkedTo<IR::Member>();
    auto hit = new IR::Member(mem->expr, IR::Type_Table::hit);
    statement->condition = hit;
    auto e = statement->ifFalse;
    if (!e) e = new IR::EmptyStatement();
    statement->ifFalse = statement->ifTrue;
    statement->ifTrue = e;
    return statement;
}

}  // namespace P4
