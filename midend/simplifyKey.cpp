/*
Copyright 2016 VMware, Inc.

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

#include "simplifyKey.h"

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/sideEffects.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/cstring.h"
#include "lib/log.h"
#include "lib/map.h"

namespace P4 {

bool IsValid::isSimple(const IR::Expression *expression, const Visitor::Context *) {
    if (!expression->is<IR::MethodCallExpression>()) return false;
    auto mi = MethodInstance::resolve(expression->to<IR::MethodCallExpression>(), refMap, typeMap);
    if (!mi->is<BuiltInMethod>()) return false;
    auto bi = mi->to<BuiltInMethod>();
    if (bi->name.name == IR::Type_Header::isValid) {
        // isValid() is simple when applied to headers, but complicated when applied to unions.
        auto baseType = typeMap->getType(bi->appliedTo, true);
        if (baseType->is<IR::Type_HeaderUnion>()) return false;
        return true;
    }
    return false;
}

const IR::Node *DoSimplifyKey::postorder(IR::KeyElement *element) {
    LOG1("Key element " << element);
    bool simple = key_policy->isSimple(element->expression, getContext());
    if (simple) return element;

    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);
    TableInsertions *insertions;
    auto it = toInsert.find(table);
    if (it == toInsert.end()) {
        insertions = new TableInsertions();
        toInsert.emplace(table, insertions);
    } else {
        insertions = it->second;
    }

    auto tmp = refMap->newName("key");
    auto type = typeMap->getType(element->expression, true);
    auto decl = new IR::Declaration_Variable(tmp, type, nullptr);
    insertions->declarations.push_back(decl);
    auto left = new IR::PathExpression(type, new IR::Path(tmp));
    auto right = element->expression;
    auto assign = new IR::AssignmentStatement(element->expression->srcInfo, left, right);
    insertions->statements.push_back(assign);

    auto path = new IR::PathExpression(type, new IR::Path(tmp));
    // This preserves annotations on the key
    element->expression = path;
    LOG2("Created new key expression " << element);
    return element;
}

const IR::Node *DoSimplifyKey::postorder(IR::P4Table *table) {
    auto insertions = ::get(toInsert, getOriginal<IR::P4Table>());
    if (insertions == nullptr) return table;

    auto result = new IR::IndexedVector<IR::Declaration>();
    for (auto d : insertions->declarations) result->push_back(d);
    result->push_back(table);
    return result;
}

const IR::Node *DoSimplifyKey::doStatement(const IR::Statement *statement,
                                           const IR::Expression *expression) {
    LOG3("Visiting " << getOriginal());
    HasTableApply hta(refMap, typeMap);
    hta.setCalledBy(this);
    (void)expression->apply(hta);
    if (hta.table == nullptr) return statement;
    auto insertions = get(toInsert, hta.table);
    if (insertions == nullptr) return statement;

    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto assign : insertions->statements) result->push_back(assign);
    result->push_back(statement);
    auto block = new IR::BlockStatement(*result);
    return block;
}

}  // namespace P4
