/*
Copyright 2021 VMware, Inc.

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

#include "keySideEffect.h"
#include "sideEffects.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

namespace {
// Checks to see whether an IR node includes a table.apply() sub-expression
class HasTableApply : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
 public:
    const IR::P4Table*  table;
    const IR::MethodCallExpression* call;
    HasTableApply(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), table(nullptr), call(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("HasTableApply"); }

    void postorder(const IR::MethodCallExpression* expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (!mi->isApply()) return;
        auto am = mi->to<P4::ApplyMethod>();
        if (!am->object->is<IR::P4Table>()) return;
        BUG_CHECK(table == nullptr, "%1% and %2%: multiple table applications in one expression",
                  table, am->object);
        table = am->object->to<IR::P4Table>();
        call = expression;
        LOG3("Invoked table is " << dbp(table));
    }
};
}  // namespace

const IR::Node* DoKeySideEffect::preorder(IR::Key* key) {
    // If any key field has side effects then pull out all
    // the key field values.
    LOG3("Visiting " << key);
    bool complex = false;
    for (auto k : key->keyElements)
        complex = complex || P4::SideEffects::check(k->expression, refMap, typeMap);
    if (!complex)
        prune();
    else
        LOG3("Will pull out " << key);
    return key;
}

const IR::Node* DoKeySideEffect::postorder(IR::KeyElement* element) {
    // If we got here we need to pull the key element out.
    LOG3("Extracting key element " << element);
    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);
    TableInsertions* insertions;
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
    auto left = new IR::PathExpression(tmp);
    auto right = element->expression;
    auto assign = new IR::AssignmentStatement(element->expression->srcInfo, left, right);
    insertions->statements.push_back(assign);

    auto path = new IR::PathExpression(tmp);
    // This preserves annotations on the key
    element->expression = path;
    LOG2("Created new key expression " << element);
    return element;
}

const IR::Node* DoKeySideEffect::postorder(IR::P4Table* table) {
    auto insertions = ::get(toInsert, getOriginal<IR::P4Table>());
    if (insertions == nullptr)
        return table;

    auto result = new IR::IndexedVector<IR::Declaration>();
    for (auto d : insertions->declarations)
        result->push_back(d);
    result->push_back(table);
    return result;
}

const IR::Node* DoKeySideEffect::doStatement(const IR::Statement* statement,
                                             const IR::Expression *expression) {
    LOG3("Visiting " << getOriginal());
    HasTableApply hta(refMap, typeMap);
    (void)expression->apply(hta);
    if (hta.table == nullptr)
        return statement;
    auto insertions = get(toInsert, hta.table);
    if (insertions == nullptr)
        return statement;

    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto assign : insertions->statements)
        result->push_back(assign);
    result->push_back(statement);
    auto block = new IR::BlockStatement(*result);
    return block;
}

}  // namespace P4
