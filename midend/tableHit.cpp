/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "tableHit.h"

#include "frontends/p4/tableApply.h"

namespace P4 {

const IR::Node *DoTableHit::process(IR::AssignmentStatement *statement, DoTableHit::op_t op) {
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
