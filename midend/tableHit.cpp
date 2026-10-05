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

    auto srcInfo = statement->srcInfo;
    const IR::Statement *tstat, *fstat;
    switch (op) {
        case None:
            tstat =
                new IR::AssignmentStatement(srcInfo, statement->left->clone(),
                                            new IR::BoolLiteral(statement->right->srcInfo, true));
            fstat = new IR::AssignmentStatement(
                srcInfo, statement->left, new IR::BoolLiteral(statement->right->srcInfo, false));
            break;
        case And:
            tstat = new IR::EmptyStatement(srcInfo);
            fstat = new IR::AssignmentStatement(
                srcInfo, statement->left, new IR::BoolLiteral(statement->right->srcInfo, false));
            break;
        case Or:
            tstat = new IR::AssignmentStatement(
                srcInfo, statement->left, new IR::BoolLiteral(statement->right->srcInfo, true));
            fstat = new IR::EmptyStatement(srcInfo);
            break;
        case Xor:
            tstat = new IR::BXorAssign(srcInfo, statement->left,
                                       new IR::BoolLiteral(statement->right->srcInfo, true));
            fstat = new IR::EmptyStatement(srcInfo);
            break;
        default:
            BUG("invalid op_t in DoTableHit");
    }
    if (negated)
        return new IR::IfStatement(srcInfo, right, fstat, tstat);
    else
        return new IR::IfStatement(srcInfo, right, tstat, fstat);
}

}  // namespace P4
