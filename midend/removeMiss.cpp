/*
Copyright 2019 VMware, Inc.

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

#include "removeMiss.h"

#include "frontends/p4/tableApply.h"

namespace P4 {

const IR::Node *DoRemoveMiss::preorder(IR::Member *expression) {
    if (!TableApplySolver::isMiss(expression, refMap, typeMap)) return expression;
    auto hit = new IR::Member(expression->expr, IR::Type_Table::hit);
    return new IR::LNot(hit);
}

const IR::Node *DoRemoveMiss::preorder(IR::IfStatement *statement) {
    if (!TableApplySolver::isMiss(statement->condition, refMap, typeMap)) return statement;
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
