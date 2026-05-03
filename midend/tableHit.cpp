// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tableHit.h"

#include "frontends/p4/tableApply.h"

namespace P4 {

const IR::Node *DoTableHit::process(IR::BaseAssignmentStatement *statement, DoTableHit::op_t op) {
    LOG3("Visiting " << getOriginal());
    auto right = statement->right;
    bool negated = false;
    if (auto neg = right->to<IR::LNot>()) {
        // We handle !hit, which may have been created by the removal of miss
        negated = true;
        right = neg->expr;
    }

    if (!TableApplySolver::isHit(right, this, typeMap)) return statement;

    const IR::Statement *tstat, *fstat;
    switch (op) {
        case None:
            tstat =
                new IR::AssignmentStatement(statement->left->clone(), new IR::BoolLiteral(true));
            fstat = new IR::AssignmentStatement(statement->left, new IR::BoolLiteral(false));
            break;
        case And:
            tstat = new IR::EmptyStatement;
            fstat = new IR::AssignmentStatement(statement->left, new IR::BoolLiteral(false));
            break;
        case Or:
            tstat = new IR::AssignmentStatement(statement->left, new IR::BoolLiteral(true));
            fstat = new IR::EmptyStatement;
            break;
        case Xor:
            tstat = new IR::BXorAssign(statement->left, new IR::BoolLiteral(true));
            fstat = new IR::EmptyStatement;
            break;
        default:
            BUG("invalid op_t in DoTableHit");
    }
    if (negated)
        return new IR::IfStatement(right, fstat, tstat);
    else
        return new IR::IfStatement(right, tstat, fstat);
}

}  // namespace P4
