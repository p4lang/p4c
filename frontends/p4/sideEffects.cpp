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

#include "frontends/p4/sideEffects.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/cloner.h"

namespace P4 {

namespace {

// Data structure used for making explicit the order of evaluation of
// sub-expressions in an expression.
// An expression e will be represented as a sequence of temporary declarations,
// followed by a sequence of statements 'statements' (mostly assignments to the temporaries,
// but which could also include conditionals for short-circuit evaluation).
struct EvaluationOrder {
    ReferenceMap* refMap;
    const IR::Expression* final;  // output
    // Declaration instead of Declaration_Variable so it can be more easily inserted
    // in the program IR.
    IR::IndexedVector<IR::Declaration> *temporaries;
    IR::IndexedVector<IR::StatOrDecl> *statements;

    explicit EvaluationOrder(ReferenceMap* refMap) :
            refMap(refMap), final(nullptr),
            temporaries(new IR::IndexedVector<IR::Declaration>()),
            statements(new IR::IndexedVector<IR::StatOrDecl>())
    { CHECK_NULL(refMap); }
    bool simple() const
    { return temporaries->empty() && statements->empty(); }

    cstring createTemporary(const IR::Type* type) {
        auto tmp = refMap->newName("tmp");
        auto decl = new IR::Declaration_Variable(
            Util::SourceInfo(), IR::ID(tmp, nullptr), IR::Annotations::empty, type, nullptr);
        temporaries->push_back(decl);
        return tmp;
    }

    const IR::Expression* addAssignment(
        cstring varName, const IR::Expression* expression) {
        auto left = new IR::PathExpression(IR::ID(varName, nullptr));
        auto stat = new IR::AssignmentStatement(Util::SourceInfo(), left, expression);
        statements->push_back(stat);
        auto result = left->clone();
        return result;
    }
};

class DismantleExpression : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    EvaluationOrder *result;
    bool leftValue;  // true when we are dismantling a left-value.
    bool resultNotUsed;  // true when the caller does not want the result (i.e.,
                         // we are invoked from a MethodCallStatement).

    // catch-all case
    const IR::Node* postorder(IR::Expression* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto orig = getOriginal<IR::Expression>();
        auto type = typeMap->getType(orig, true);
        typeMap->setType(expression, type);
        if (typeMap->isLeftValue(orig))
            typeMap->setLeftValue(expression);
        if (typeMap->isCompileTimeConstant(orig))
            typeMap->setCompileTimeConstant(expression);
        result->final = expression;
        return result->final;
    }

    const IR::Node* preorder(IR::Literal* expression) override {
        result->final = expression;
        prune();
        return expression;
    }

    const IR::Node* preorder(IR::ArrayIndex* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto left = result->final;
            CHECK_NULL(left);
            bool save = leftValue;
            leftValue = false;
            visit(expression->right);
            auto right = result->final;
            CHECK_NULL(right);
            leftValue = save;
            if (!right->is<IR::Constant>()) {
                auto indexType = typeMap->getType(expression->right, true);
                auto tmp = result->createTemporary(indexType);
                right = result->addAssignment(tmp, right);
                typeMap->setType(right, indexType);
            }

            result->final = new IR::ArrayIndex(expression->srcInfo, left, right);
        }
        typeMap->setType(result->final, type);
        if (leftValue)
            typeMap->setLeftValue(result->final);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Member* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->expr);
            auto left = result->final;
            CHECK_NULL(left);
            result->final = new IR::Member(expression->srcInfo, left, expression->member);
            typeMap->setType(result->final, type);
            if (leftValue)
                typeMap->setLeftValue(result->final);

            // Special case for table.apply().hit, which is not dismantled by
            // the MethodCallExpression.
            if (TableApplySolver::isHit(expression, refMap, typeMap)) {
                BUG_CHECK(type->is<IR::Type_Boolean>(), "%1%: not boolean", type);
                auto tmp = result->createTemporary(type);
                auto path = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto stat = new IR::AssignmentStatement(Util::SourceInfo(), path, result->final);
                result->statements->push_back(stat);
                result->final = path->clone();
                typeMap->setType(result->final, type);
            }
            prune();
            return result->final;
        }
        typeMap->setType(result->final, type);
        if (leftValue)
            typeMap->setLeftValue(result->final);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::SelectExpression* expression) override {
        LOG3("Visiting " << dbp(expression));
        visit(expression->select);
        prune();
        result->final = expression;
        return expression;
    }

    const IR::Node* preorder(IR::Operation_Unary* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        visit(expression->expr);
        auto left = result->final;
        CHECK_NULL(left);
        auto clone = expression->clone();
        clone->expr = left;
        typeMap->setType(clone, type);
        result->final = clone;
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Operation_Binary* expression) override {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto left = result->final;
            CHECK_NULL(left);
            visit(expression->right);
            auto right = result->final;
            auto clone = expression->clone();
            clone->left = left;
            clone->right = right;
            typeMap->setType(clone, type);
            auto tmp = result->createTemporary(type);
            auto path = result->addAssignment(tmp, clone);
            result->final = path;
        }
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* shortCircuit(IR::Operation_Binary* expression) {
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        if (!SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
            result->final = expression;
        } else {
            visit(expression->left);
            auto cond = result->final;
            CHECK_NULL(cond);

            // e1 && e2
            // becomes roughly:
            // if (!simplify(e1))
            //    tmp = false;
            // else
            //    tmp = simplify(e2);

            bool land = expression->is<IR::LAnd>();
            auto constant = new IR::BoolLiteral(Util::SourceInfo(), !land);
            auto tmp = result->createTemporary(type);
            auto ifTrue = new IR::AssignmentStatement(
                Util::SourceInfo(), new IR::PathExpression(IR::ID(tmp, nullptr)), constant);
            auto ifFalse = new IR::IndexedVector<IR::StatOrDecl>();

            auto save = result->statements;
            result->statements = ifFalse;
            visit(expression->right);
            auto path = result->addAssignment(tmp, result->final);
            result->statements = save;
            if (land) {
                cond = new IR::LNot(Util::SourceInfo(), cond);
                typeMap->setType(cond, type);
            }
            auto block = new IR::BlockStatement(Util::SourceInfo(), IR::Annotations::empty, ifFalse);
            auto ifStatement = new IR::IfStatement(Util::SourceInfo(), cond, ifTrue, block);
            result->statements->push_back(ifStatement);
            result->final = path->clone();
        }
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::Mux* expression) override {
        // We always dismantle muxes - some architectures may not support them
        LOG3("Visiting " << dbp(expression));
        auto type = typeMap->getType(getOriginal(), true);
        visit(expression->e0);
        auto e0 = result->final;
        CHECK_NULL(e0);
        auto tmp = result->createTemporary(type);

        auto save = result->statements;
        auto ifTrue = new IR::IndexedVector<IR::StatOrDecl>();
        result->statements = ifTrue;
        visit(expression->e1);
        (void)result->addAssignment(tmp, result->final);

        auto ifFalse = new IR::IndexedVector<IR::StatOrDecl>();
        result->statements = ifFalse;
        visit(expression->e2);
        auto path = result->addAssignment(tmp, result->final);
        result->statements = save;

        auto ifStatement = new IR::IfStatement(
            Util::SourceInfo(), e0,
            new IR::BlockStatement(Util::SourceInfo(), IR::Annotations::empty, ifTrue),
            new IR::BlockStatement(Util::SourceInfo(), IR::Annotations::empty, ifFalse));
        result->statements->push_back(ifStatement);
        result->final = path->clone();
        typeMap->setType(result->final, type);
        prune();
        return result->final;
    }

    const IR::Node* preorder(IR::LAnd* expression) override { return shortCircuit(expression); }
    const IR::Node* preorder(IR::LOr* expression) override { return shortCircuit(expression); }

    // We don't want to compute the full read/write set here so we
    // overapproximate it as follows: all declarations that occur in
    // an expression.  TODO: this could be made more precise, perhaps
    // using LocationSets.
    class ReadsWrites : public Inspector {
     public:
        std::set<const IR::IDeclaration*> decls;
        ReferenceMap* refMap;

        explicit ReadsWrites(ReferenceMap* refMap) : refMap(refMap)
        { setName("ReadsWrites"); }

        void postorder(const IR::PathExpression* expression) override {
            auto decl = refMap->getDeclaration(expression->path);
            decls.emplace(decl);
        }
    };

    bool mayAlias(const IR::Expression* left, const IR::Expression* right) const {
        ReadsWrites rwleft(refMap);
        (void)left->apply(rwleft);
        ReadsWrites rwright(refMap);
        (void)right->apply(rwright);

        for (auto d : rwleft.decls) {
            if (rwright.decls.count(d) > 0) {
                LOG3(dbp(d) << " accessed by both " << dbp(left) << " and " << dbp(right));
                return true;
            }
        }
        return false;
    }

    const IR::Node* preorder(IR::MethodCallExpression* mce) override {
        BUG_CHECK(!leftValue, "%1%: method on left hand side?", mce);
        LOG3("Visiting " << dbp(mce));
        auto orig = getOriginal<IR::MethodCallExpression>();
        auto type = typeMap->getType(orig, true);
        if (!SideEffects::check(orig, refMap, typeMap)) {
            result->final = mce;
            return mce;
        }

        auto copyBack = new IR::IndexedVector<IR::StatOrDecl>();
        auto args = new IR::Vector<IR::Expression>();
        MethodCallDescription desc(orig, refMap, typeMap);
        bool savelv = leftValue;
        bool savenu = resultNotUsed;
        resultNotUsed = false;

        // If a parameter is in this set then we use a temporary to
        // copy the corresponding argument.  We could always use
        // temporaries for arguments - it is always correct - but this
        // could entail the creation of "fat" temporaries that contain
        // large structs.  We want to avoid copying these large
        // structs if possible.
        std::set<const IR::Parameter*> useTemporary;

        bool anyOut = false;
        for (auto p : *desc.substitution.getParameters()) {
            if (p->direction == IR::Direction::None)
                continue;
            if (p->hasOut())
                anyOut = true;
            auto arg = desc.substitution.lookup(p);
            // If an argument evaluation has side-effects then
            // always use a temporary to hold the argument value.
            if (SideEffects::check(arg, refMap, typeMap)) {
                LOG3("Using temporary for " << dbp(mce) <<
                     " param " << dbp(p) << " arg side effect");
                useTemporary.emplace(p);
            }
        }

        if (anyOut) {
            // Check aliasing between all pairs of arguments where at
            // least one of them is out or inout.
            for (auto p1 : *desc.substitution.getParameters()) {
                auto arg1 = desc.substitution.lookup(p1);
                for (auto p2 : *desc.substitution.getParameters()) {
                    if (p2 == p1)
                        break;
                    if (!p1->hasOut() && !p2->hasOut())
                        continue;
                    auto arg2 = desc.substitution.lookup(p2);
                    if (mayAlias(arg1, arg2)) {
                        LOG3("Using temporary for " << dbp(mce) <<
                             " param " << dbp(p1) << " aliasing" << dbp(p2));
                        useTemporary.emplace(p1);
                        useTemporary.emplace(p2);
                        break;
                    }
                }
            }
        }

        visit(mce->method);
        auto method = result->final;

        ClonePathExpressions cloner;  // a cheap version of deep copy
        for (auto p : *desc.substitution.getParameters()) {
            auto arg = desc.substitution.lookup(p);
            if (p->direction == IR::Direction::None) {
                args->push_back(arg);
                continue;
            }

            bool useTemp = useTemporary.count(p) != 0;
            LOG3("Transforming " << dbp(arg) << " for " << dbp(p) <<
                 (useTemp ? " with " : " without ") << "temporary");

            if (p->direction == IR::Direction::In)
                leftValue = false;
            else
                leftValue = true;
            auto paramtype = typeMap->getType(p, true);
            const IR::Expression* argValue;
            visit(arg);  // May mutate arg!  Recursively simplifies arg.
            auto newarg = result->final;
            CHECK_NULL(newarg);

            if (useTemp) {
                // declare temporary variable
                auto tmp = refMap->newName("tmp");
                argValue = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto decl = new IR::Declaration_Variable(
                    Util::SourceInfo(), IR::ID(tmp, nullptr),
                    IR::Annotations::empty, paramtype, nullptr);
                result->temporaries->push_back(decl);
                if (p->direction != IR::Direction::Out) {
                    auto clone = argValue->clone();
                    auto stat = new IR::AssignmentStatement(
                        Util::SourceInfo(), clone, newarg);
                    LOG3(clone << " = " << newarg);
                    result->statements->push_back(stat);
                    typeMap->setType(clone, paramtype);
                    typeMap->setLeftValue(clone);
                }
            } else {
                argValue = newarg;
            }
            if (leftValue && useTemp) {
                auto assign = new IR::AssignmentStatement(
                    Util::SourceInfo(), cloner.clone<IR::Expression>(newarg),
                    cloner.clone<IR::Expression>(argValue));
                copyBack->push_back(assign);
                LOG3("Will copy out value " << dbp(assign));
            }
            args->push_back(argValue);
        }
        leftValue = savelv;
        resultNotUsed = savenu;

        // Special handling for table.apply(...).X; we cannot generate
        // a temporary for the method call tmp = table.apply(), since
        // we cannot write down the type of tmp.  So we don't
        // dismantle these expressions.
        bool tbl_apply = false;
        auto ctx = getContext();
        if (ctx != nullptr && ctx->node->is<IR::Member>()) {
            auto mmbr = ctx->node->to<IR::Member>();
            auto tbl = TableApplySolver::isActionRun(mmbr, refMap, typeMap);
            auto tbl1 = TableApplySolver::isHit(mmbr, refMap, typeMap);
            tbl_apply = tbl != nullptr || tbl1 != nullptr;
        }
        // Simplified method call, with arguments substituted
        auto simplified = new IR::MethodCallExpression(
            mce->srcInfo, method, mce->typeArguments, args);
        typeMap->setType(simplified, type);
        result->final = simplified;
        // See whether we assign the result of the call to a temporary
        if (!type->is<IR::Type_Void>() &&  // no return type
            !tbl_apply &&                  // not a table.apply call
            !resultNotUsed) {              // result of call is not used
            auto tmp = refMap->newName("tmp");
            auto decl = new IR::Declaration_Variable(
                Util::SourceInfo(), IR::ID(tmp, nullptr), IR::Annotations::empty,
                type, nullptr);
            result->temporaries->push_back(decl);
            auto left = new IR::PathExpression(IR::ID(tmp, nullptr));
            auto stat = new IR::AssignmentStatement(
                Util::SourceInfo(), left, simplified);
            result->statements->push_back(stat);
            result->final = left->clone();
            typeMap->setType(result->final, type);
            LOG3(mce << " replaced with " << left << " = " << simplified);
        } else {
            if (tbl_apply) {
                result->final = simplified;
            } else {
                result->statements->push_back(
                    new IR::MethodCallStatement(mce->srcInfo, simplified));
                result->final = nullptr;
            }
        }
        result->statements->append(*copyBack);
        prune();
        return result->final;
    }

 public:
    DismantleExpression(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), leftValue(false) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        result = new EvaluationOrder(refMap);
        setName("DismantleExpressions");
    }
    EvaluationOrder* dismantle(const IR::Expression* expression,
                               bool isLeftValue, bool resultNotUsed = false) {
        LOG3("Dismantling " << dbp(expression) << (isLeftValue ? " on left" : " on right"));
        leftValue = isLeftValue;
        this->resultNotUsed = resultNotUsed;
        (void)expression->apply(*this);
        LOG3("Result is " << result->final);
        return result;
    }
};
}  // namespace

const IR::Node* DoSimplifyExpressions::postorder(IR::Function* function) {
    if (toInsert.empty())
        return function;
    auto locals = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto a : toInsert)
        locals->push_back(a);
    for (auto s : *function->body->components)
        locals->push_back(s);
    function->body = new IR::BlockStatement(
        function->body->srcInfo, IR::Annotations::empty, locals);
    toInsert.clear();
    return function;
}

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
    action->body = new IR::BlockStatement(action->body->srcInfo, IR::Annotations::empty, locals);
    toInsert.clear();
    return action;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ParserState* state) {
    if (state->selectExpression == nullptr)
        return state;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(state->selectExpression, false);
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
    auto left = dm.dismantle(statement->left, true)->final;
    CHECK_NULL(left);
    auto parts = dm.dismantle(statement->right, false);
    CHECK_NULL(parts);
    toInsert.append(*parts->temporaries);
    auto right = parts->final;
    CHECK_NULL(right);
    parts->statements->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
    auto block = new IR::BlockStatement(
        Util::SourceInfo(), IR::Annotations::empty, parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::MethodCallStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->methodCall, false, true);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto block = new IR::BlockStatement(
        Util::SourceInfo(), IR::Annotations::empty, parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ReturnStatement* statement) {
    if (statement->expression == nullptr)
        return statement;
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::ReturnStatement(statement->srcInfo, expr));
    auto block = new IR::BlockStatement(
        Util::SourceInfo(), IR::Annotations::empty, parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::IfStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->condition, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(new IR::IfStatement(statement->srcInfo, expr,
                                                     statement->ifTrue, statement->ifFalse));
    auto block = new IR::BlockStatement(
        Util::SourceInfo(), IR::Annotations::empty, parts->statements);
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::SwitchStatement* statement) {
    DismantleExpression dm(refMap, typeMap);
    auto parts = dm.dismantle(statement->expression, false);
    CHECK_NULL(parts);
    if (parts->simple())
        return statement;
    toInsert.append(*parts->temporaries);
    auto expr = parts->final;
    parts->statements->push_back(
        new IR::SwitchStatement(statement->srcInfo, expr, std::move(statement->cases)));
    auto block = new IR::BlockStatement(
        Util::SourceInfo(), IR::Annotations::empty, parts->statements);
    return block;
}

}  // namespace P4
