#include "simplifyKey.h"
#include "removeParameters.h"  // for HasTableApply
#include "frontends/p4/methodInstance.h"

namespace P4 {

bool NonLeftValue::isTooComplex(const IR::Expression* expression) const {
    if (typeMap->isLeftValue(expression))
        return false;
    if (!expression->is<IR::MethodCallExpression>())
        return true;
    auto mi = MethodInstance::resolve(expression->to<IR::MethodCallExpression>(), refMap, typeMap);
    if (!mi->is<BuiltInMethod>())
        return true;
    auto bi = mi->to<BuiltInMethod>();
    if (bi->name.name == IR::Type_Header::isValid)
        return false;
    return true;
}

const IR::Node* DoSimplifyKey::postorder(IR::KeyElement* element) {
    bool doSimplify = policy->isTooComplex(element->expression);
    if (!doSimplify)
        return element;

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
    auto decl = new IR::Declaration_Variable(
        Util::SourceInfo(), tmp, IR::Annotations::empty,
        type, nullptr);
    insertions->declarations.push_back(decl);
    auto left = new IR::PathExpression(tmp);
    auto right = element->expression;
    auto assign = new IR::AssignmentStatement(element->expression->srcInfo, left, right);
    insertions->statements.push_back(assign);

    auto path = new IR::PathExpression(tmp);
    // This preserves annotations on the key
    element->expression = path;
    return element;
}

const IR::Node* DoSimplifyKey::postorder(IR::P4Table* table) {
    auto insertions = ::get(toInsert, getOriginal<IR::P4Table>());
    if (insertions == nullptr)
        return table;

    auto result = new IR::IndexedVector<IR::Declaration>();
    for (auto d : insertions->declarations)
        result->push_back(d);
    result->push_back(table);
    return result;
}

const IR::Node* DoSimplifyKey::doStatement(const IR::Statement* statement,
                                         const IR::Expression *expression) {
    LOG1("Visiting " << getOriginal());
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
    return result;
}

}  // namespace P4
