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

#include <ostream>

#include "frontends/p4/tableApply.h"
#include "lib/log.h"

namespace P4 {

const IR::Node *DoTableHit::postorder(IR::AssignmentStatement *statement) {
    LOG3("Visiting " << getOriginal());
    auto right = statement->right;
    bool negated = false;
    if (auto neg = right->to<IR::LNot>()) {
        // We handle !hit, which may have been created by the removal of miss
        negated = true;
        right = neg->expr;
    }

    if (!TableApplySolver::isHit(right, refMap, typeMap)) return statement;

    auto tstat = new IR::AssignmentStatement(statement->left->clone(), new IR::BoolLiteral(true));
    auto fstat = new IR::AssignmentStatement(statement->left->clone(), new IR::BoolLiteral(false));
    if (negated)
        return new IR::IfStatement(right, fstat, tstat);
    else
        return new IR::IfStatement(right, tstat, fstat);
}

}  // namespace P4
