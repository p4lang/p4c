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

#include "simplifyExpressions.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/tableApply.h"

namespace P4 {

namespace {
// An expression e will be represented as a sequence of temporary declarations,
// followed by a sequence of assignments to the temporaries, followed by
// an expression involving the temporaries.
// For example: e(f(), 2, g())
// is turned into
// tmp1 = f()
// tmp2 = g()
// e(tmp1, 2, tmp2)
struct ExpressionParts {
    const IR::Expression* final;  // simple expression
    IR::IndexedVector<IR::Declaration> *temporaries;
    IR::IndexedVector<IR::StatOrDecl> *statements;

    ExpressionParts() : final(nullptr), temporaries(new IR::IndexedVector<IR::Declaration>()),
                        statements(new IR::IndexedVector<IR::StatOrDecl>()) {}
    bool simple() const
    { return temporaries->empty() && statements->empty(); }
};

class DismantleExpression : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    ExpressionParts *result;

    const IR::Node* postorder(IR::Expression* expression) override {
        LOG1("Visiting " << expression);
        auto type = typeMap->getType(getOriginal(), true);
        typeMap->setType(expression, type);
        result->final = expression;
        return expression;
    }

    const IR::Node* postorder(IR::Member* expression) override {
        LOG1("Visiting " << expression);
        auto tbl = TableApplySolver::isHit(expression, refMap, typeMap);
        auto ctx = getContext();
        if (tbl == nullptr || ctx == nullptr)
            return expression;

        auto type = typeMap->getType(getOriginal(), true);
        auto tmp = refMap->newName("tmp");
        auto decl = new IR::Declaration_Variable(
            Util::SourceInfo(), IR::ID(tmp), IR::Annotations::empty,
            type, nullptr);
        result->temporaries->push_back(decl);
        auto left = new IR::PathExpression(IR::ID(tmp));
        auto stat = new IR::AssignmentStatement(
            Util::SourceInfo(), left, expression);
        result->statements->push_back(stat);
        result->final = left->clone();
        LOG1(expression << " replaced with " << left << " = " << expression);
        typeMap->setType(result->final, type);
        return result->final;
    }

    const IR::Node* postorder(IR::MethodCallExpression* mce) override {
        LOG1("Visiting " << mce);
        auto orig = getOriginal<IR::MethodCallExpression>();
        if (!SideEffects::check(orig, refMap, typeMap)) {
            result->final = mce;
            return mce;
        }
        // Special handling for table.apply(...).X;
        // we cannot generate a temporary for the apply:
        // tmp = table.apply(), since we cannot write down the type of tmp
        bool tbl_apply = false;
        auto ctx = getContext();
        if (ctx != nullptr && ctx->node->is<IR::Member>()) {
            auto mmbr = ctx->node->to<IR::Member>();
            auto tbl = TableApplySolver::isActionRun(mmbr, refMap, typeMap);
            auto tbl1 = TableApplySolver::isHit(mmbr, refMap, typeMap);
            tbl_apply = tbl != nullptr || tbl1 != nullptr;
        }
        auto type = typeMap->getType(getOriginal(), true);
        if (!type->is<IR::Type_Void>() &&
            !tbl_apply &&
            ctx != nullptr) {  // ctx is nullptr if the method call is toplevel.
            auto tmp = refMap->newName("tmp");
            auto decl = new IR::Declaration_Variable(
                Util::SourceInfo(), IR::ID(tmp), IR::Annotations::empty,
                type, nullptr);
            result->temporaries->push_back(decl);
            auto left = new IR::PathExpression(IR::ID(tmp));
            auto stat = new IR::AssignmentStatement(
                Util::SourceInfo(), left, mce);
            result->statements->push_back(stat);
            result->final = left->clone();
            typeMap->setType(result->final, type);
            LOG1(mce << " replaced with " << left << " = " << mce);
        } else {
            result->final = mce;
        }
        return result->final;
    }

 public:
    DismantleExpression(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        result = new ExpressionParts();
        setName("DismantleExpressions");
    }
    ExpressionParts* dismantle(const IR::Expression* expression) {
        LOG1("Dismantling " << expression);
        result->final = expression->apply(*this)->to<IR::Expression>();
        return result;
    }
};
}  // namespace

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Parser* parser) {
    if (toInsert.empty())
        return parser;
    auto locals = new IR::IndexedVector<IR::Declaration>(*parser->parserLocals);
    locals->append(toInsert);
    parser->parserLocals = locals;
    toInsert.clear();
    return parser;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Control* control) {
    if (toInsert.empty())
        return control;
    auto locals = new IR::IndexedVector<IR::Declaration>(*control->controlLocals);
    locals->append(toInsert);
    control->controlLocals = locals;
    toInsert.clear();
    return control;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Action* action) {
    if (toInsert.empty())
        return action;
    auto locals = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto a : toInsert)
        locals->push_back(a);
    for (auto s : *action->body->components)
        locals->push_back(s);
    action->body = new IR::BlockStatement(action->body->srcInfo, locals);
    toInsert.clear();
    return action;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ParserState* state) {
    if (state->selectExpression == nullptr)
        return state;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(state->selectExpression);
    CHECK_NULL(parts);
    if (parts->simple())
        return state;
    toInsert.append(*parts->temporaries);
    auto comp = new IR::IndexedVector<IR::StatOrDecl>(*state->components);
    comp->append(*parts->statements);
    state->components = comp;
    state->selectExpression = parts->final;
    return state;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::AssignmentStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto left = dm.dismantle(statement->left)->final;
    auto parts = dm.dismantle(statement->right);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto right = parts->final;
    if (findContext<IR::ParserState>()) {
        // in a state we cannot have block statements
        return parts->statements;
    } else {
        parts->statements->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
        auto block = new IR::BlockStatement(Util::SourceInfo(), parts->statements);
        return block;
    }
}

const IR::Node* DoSimplifyExpressions::postorder(IR::MethodCallStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->methodCall);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto right = parts->final->to<IR::MethodCallExpression>();
    parts->statements->push_back(new IR::MethodCallStatement(statement->srcInfo, right));
    if (findContext<IR::ParserState>()) {
        // in a state we cannot have block statements
        return parts->statements;
    } else {
        auto block = new IR::BlockStatement(Util::SourceInfo(), parts->statements);
        return block;
    }
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ReturnStatement* statement) {
    if (statement->expression == nullptr)
        return statement;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::ReturnStatement(statement->srcInfo, expr));
    auto block = new IR::BlockStatement(Util::SourceInfo(), parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::IfStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->condition);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::IfStatement(statement->srcInfo, expr,
                                                     statement->ifTrue, statement->ifFalse));
    auto block = new IR::BlockStatement(Util::SourceInfo(), parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::SwitchStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(
        new IR::SwitchStatement(statement->srcInfo, expr, std::move(statement->cases)));
    auto block = new IR::BlockStatement(Util::SourceInfo(), parts->statements);
    return block;
}

}  // namespace P4
