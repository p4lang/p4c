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
#include "frontends/p4/alias.h"

namespace P4 {

cstring DoSimplifyExpressions::createTemporary(const IR::Type* type) {
    type = type->getP4Type();
    BUG_CHECK(!type->is<IR::Type_Dontcare>(), "Can't create don't-care temps");
    auto tmp = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(tmp, nullptr), type);
    toInsert.push_back(decl);
    return tmp;
}

/** Add ```@varName = @expression``` to the vector of statements.
  *
  * @return A copy of the l-value expression created for varName.
  */
const IR::Expression* DoSimplifyExpressions::addAssignment(
    Util::SourceInfo srcInfo,
    cstring varName,
    const IR::Expression* expression) {
    auto left = new IR::PathExpression(IR::ID(varName, nullptr));
    auto stat = new IR::AssignmentStatement(srcInfo, left, expression);
    statements.push_back(stat);
    auto result = left->clone();
    return result;
}

// catch-all case
const IR::Node* DoSimplifyExpressions::postorder(IR::Expression* expression) {
    LOG3("Visiting " << dbp(expression));
    auto orig = getOriginal<IR::Expression>();
    typeMap->cloneExpressionProperties(expression, orig);
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::Literal* expression) {
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::ArrayIndex* expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    if (SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
        visit(expression->left);
        CHECK_NULL(expression->left);
        visit(expression->right);
        CHECK_NULL(expression->right);
        if (!expression->right->is<IR::Constant>()) {
            auto indexType = typeMap->getType(expression->right, true);
            auto tmp = createTemporary(indexType);
            expression->right = addAssignment(expression->srcInfo, tmp, expression->right);
            typeMap->setType(expression->right, indexType);
        }
    }
    typeMap->setType(expression, type);
    if (isWrite())
        typeMap->setLeftValue(expression);
    prune();
    return expression;
}

static bool isIfContext(const Visitor::Context *ctxt) {
    if (ctxt && ctxt->node->is<IR::LNot>())
        ctxt = ctxt->parent;
    return ctxt && ctxt->node->is<IR::IfStatement>();
}

const IR::Node* DoSimplifyExpressions::preorder(IR::Member* expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    const IR::Expression *rv = expression;
    if (SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
        visit(expression->expr);
        CHECK_NULL(expression->expr);

        // Special case for table.apply().hit/miss, which is not dismantled by
        // the MethodCallExpression.
        if (TableApplySolver::isHit(expression, refMap, typeMap) ||
            TableApplySolver::isMiss(expression, refMap, typeMap)) {
            if (isIfContext(getContext())) {
                /* if the hit/miss test is directly in an "if", don't bother cloning it
                 * as that will just create a redundant table later */
            } else if (getParent<IR::AssignmentStatement>()) {
                /* already assigning it somewhere -- no need to add another copy */
            } else {
                BUG_CHECK(type->is<IR::Type_Boolean>(), "%1%: not boolean", type);
                auto tmp = createTemporary(type);
                auto path = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto stat = new IR::AssignmentStatement(path, expression);
                statements.push_back(stat);
                typeMap->setType(expression, type);
                rv = path->clone();;
            }
        }
    }
    typeMap->setType(rv, type);
    if (isWrite())
        typeMap->setLeftValue(rv);
    prune();
    return rv;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::SelectExpression* expression) {
    LOG3("Visiting " << dbp(expression));
    visit(expression->select);
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::Operation_Unary* expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    visit(expression->expr);
    CHECK_NULL(expression->expr);
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::StructExpression* expression) {
    LOG3("Visiting " << dbp(expression));
    bool foundEffect = false;
    for (auto v : expression->components) {
        if (SideEffects::check(v->expression, refMap, typeMap)) {
            foundEffect = true;
            break;
        }
    }
    if (!foundEffect)
        return expression;
    // allocate temporaries for all members in order.
    // this will handle cases like a = (S) { b, f(b) }, where f can mutate b.
    IR::IndexedVector<IR::NamedExpression> vec;
    LOG3("Dismantling " << dbp(expression));
    for (auto &v : expression->components) {
        auto t = typeMap->getType(v->expression, true);
        auto tmp = createTemporary(t);
        visit(v);
        auto path = addAssignment(expression->srcInfo, tmp, v->expression);
        typeMap->setType(path, t);
        v = new IR::NamedExpression(v->name, path);
    }
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::ListExpression* expression) {
    LOG3("Visiting " << dbp(expression));
    bool foundEffect = false;
    for (auto v : expression->components) {
        if (SideEffects::check(v, refMap, typeMap)) {
            foundEffect = true;
            break;
        }
    }
    if (!foundEffect)
        return expression;
    // allocate temporaries for all members in order.
    // this will handle cases like a = { b, f(b) }, where f can mutate b.
    LOG3("Dismantling " << dbp(expression));
    for (auto &v : expression->components) {
        auto t = typeMap->getType(v, true);
        auto tmp = createTemporary(t);
        visit(v);
        auto path = addAssignment(expression->srcInfo, tmp, v);
        v = path;
        typeMap->setType(path, t);
    }
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::Operation_Binary* expression) {
    LOG3("Visiting " << dbp(expression));
    auto original = getOriginal<IR::Operation_Binary>();
    auto type = typeMap->getType(original, true);
    if (SideEffects::check(original, refMap, typeMap)) {
        if (SideEffects::check(original->right, refMap, typeMap)) {
            // We are a bit conservative here. We handle this case:
            // T f(inout T val) { ... }
            // val + f(val);
            // We must save val before the evaluation of f
            auto ltype = typeMap->getType(original->left, true);
            auto leftTmp = createTemporary(ltype);
            visit(expression->left);
            auto leftPath = addAssignment(expression->srcInfo, leftTmp, expression->left);
            expression->left = leftPath;
            typeMap->setType(leftPath, ltype);
        } else {
            visit(expression->left);
        }
        CHECK_NULL(expression->left);
        visit(expression->right);
        typeMap->setType(expression, type);
        auto tmp = createTemporary(type);
        auto path = addAssignment(expression->srcInfo, tmp, expression);
        typeMap->setType(path, type);
        prune();
        return path;
    }
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::shortCircuit(IR::Operation_Binary* expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    if (SideEffects::check(getOriginal<IR::Expression>(), refMap, typeMap)) {
        visit(expression->left);
        CHECK_NULL(expression->left);

        // e1 && e2
        // becomes roughly:
        // if (!simplify(e1))
        //    tmp = false;
        // else
        //    tmp = simplify(e2);

        bool land = expression->is<IR::LAnd>();
        auto constant = new IR::BoolLiteral(!land);
        auto tmp = createTemporary(type);
        auto ifTrue = new IR::AssignmentStatement(expression->srcInfo,
            new IR::PathExpression(IR::ID(tmp, nullptr)), constant);

        auto save = statements;
        statements.clear();
        visit(expression->right);
        auto path = addAssignment(expression->srcInfo, tmp, expression->right);
        auto ifFalse = statements;
        statements = save;
        if (land) {
            expression->left = new IR::LNot(expression->left);
            typeMap->setType(expression->left, type);
        }
        auto block = new IR::BlockStatement(ifFalse);
        auto ifStatement = new IR::IfStatement(expression->srcInfo,
                                               expression->left, ifTrue, block);
        statements.push_back(ifStatement);
        typeMap->setType(path, type);
        prune();
        return path;
    }
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::Mux* expression) {
    // We always dismantle muxes - some architectures may not support them
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    visit(expression->e0);
    CHECK_NULL(expression->e0);
    auto tmp = createTemporary(type);

    auto save = statements;
    statements.clear();
    visit(expression->e1);
    (void)addAssignment(expression->srcInfo, tmp, expression->e1);
    auto ifTrue = statements;

    statements.clear();
    visit(expression->e2);
    auto path = addAssignment(expression->srcInfo, tmp, expression->e2);
    auto ifFalse = statements;
    statements = save;

    auto ifStatement = new IR::IfStatement(
        expression->e0, new IR::BlockStatement(ifTrue), new IR::BlockStatement(ifFalse));
    statements.push_back(ifStatement);
    typeMap->setType(path, type);
    prune();
    return path;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::LAnd* expression) {
    return shortCircuit(expression); }
const IR::Node* DoSimplifyExpressions::preorder(IR::LOr* expression) {
    return shortCircuit(expression); }

bool DoSimplifyExpressions::mayAlias(const IR::Expression* left,
                                     const IR::Expression* right) const {
    ReadsWrites rw(refMap, true);
    return rw.mayAlias(left, right);
}

/// Returns true if type is a header or a struct containing a header.
/// (We don't care about stacks or unions.)
bool DoSimplifyExpressions::containsHeaderType(const IR::Type* type) {
    if (type->is<IR::Type_Header>())
        return true;
    auto st = type->to<IR::Type_Struct>();
    if (st == nullptr)
        return false;
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (containsHeaderType(ftype))
            return true;
    }
    return false;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::MethodCallExpression* mce) {
    BUG_CHECK(!isWrite(), "%1%: method on left hand side?", mce);
    LOG3("Visiting " << dbp(mce));
    auto orig = getOriginal<IR::MethodCallExpression>();
    auto type = typeMap->getType(orig, true);
    if (!SideEffects::check(orig, refMap, typeMap)) {
        return mce;
    }

    auto copyBack = new IR::IndexedVector<IR::StatOrDecl>();
    auto args = new IR::Vector<IR::Argument>();
    auto mi = MethodInstance::resolve(orig, refMap, typeMap);

    // If a parameter is in this set then we use a temporary to
    // copy the corresponding argument.  We could always use
    // temporaries for arguments - it is always correct - but this
    // could entail the creation of "fat" temporaries that contain
    // large structs.  We want to avoid copying these large
    // structs if possible.
    std::set<const IR::Parameter*> useTemporary;
    std::set<const IR::Expression*> hasSideEffects;

    bool anyOut = false;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (p->direction == IR::Direction::None)
            continue;
        if (p->hasOut())
            anyOut = true;
        auto arg = mi->substitution.lookup(p);
        // If an argument evaluation has side-effects then
        // always use a temporary to hold the argument value.
        if (SideEffects::check(arg->expression, refMap, typeMap)) {
            LOG3("Using temporary for " << dbp(mce) <<
                 " param " << dbp(p) << " arg side effect");
            useTemporary.emplace(p);
            hasSideEffects.emplace(arg->expression);
            continue;
        }
        // If the parameter contains header values and the
        // argument is a list expression or a struct initializer
        // then we also use a temporary.  This makes the job of
        // the SetHeaders pass later simpler (otherwise we have to
        // handle this case there).
        auto ptype = typeMap->getType(p, true);
        if (!containsHeaderType(ptype))
            continue;

        if (arg->expression->is<IR::ListExpression>() ||
            arg->expression->is<IR::StructExpression>()) {
            LOG3("Using temporary for " << dbp(mce) <<
                 " param " << dbp(p) << " assigning tuple to header");
            useTemporary.emplace(p);
            continue;
        }
    }

    if (anyOut) {
        // Check aliasing between all pairs of arguments where at
        // least one of them is out or inout.
        for (auto p1 : *mi->substitution.getParametersInArgumentOrder()) {
            auto arg1 = mi->substitution.lookup(p1);
            if (hasSideEffects.count(arg1->expression))
                continue;
            for (auto p2 : *mi->substitution.getParametersInArgumentOrder()) {
                LOG3("p1=" << dbp(p1) << " p2=" << dbp(p2));
                if (p2 == p1)
                    break;
                if (!p1->hasOut() && !p2->hasOut())
                    continue;
                auto arg2 = mi->substitution.lookup(p2);
                if (hasSideEffects.count(arg2->expression))
                    continue;
                if (mayAlias(arg1->expression, arg2->expression)) {
                    LOG3("Using temporary for " << dbp(mce) <<
                         " param " << dbp(p1) << " aliasing" << dbp(p2));
                    if (p1->hasOut() && p2->hasOut())
                        ::warning(ErrorType::WARN_ORDERING,
                                  "%1%: 'out' argument has fields in common with %2%", arg1, arg2);
                    useTemporary.emplace(p1);
                    useTemporary.emplace(p2);
                    break;
                }
            }
        }
    }

    visit(mce->method);

    ClonePathExpressions cloner;  // a cheap version of deep copy
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        if (p->direction == IR::Direction::None) {
            args->push_back(arg);
            continue;
        }

        bool useTemp = useTemporary.count(p) != 0;
        LOG3("Transforming " << dbp(arg) << " for " << dbp(p) <<
             (useTemp ? " with " : " without ") << "temporary");

        const IR::Expression* argValue = nullptr;
        visit(arg);  // May mutate arg!  Recursively simplifies arg.
        auto newarg = arg->expression;
        CHECK_NULL(newarg);

        if (useTemp) {
            // declare temporary variable
            auto paramtype = typeMap->getType(p, true);
            if (paramtype->is<IR::Type_Dontcare>())
                paramtype = typeMap->getType(arg, true);
            auto tmp = createTemporary(paramtype);
            argValue = new IR::PathExpression(IR::ID(tmp, nullptr));
            if (p->direction != IR::Direction::Out) {
                auto clone = argValue->clone();
                auto stat = new IR::AssignmentStatement(clone, newarg);
                LOG3(clone << " = " << newarg);
                statements.push_back(stat);
                typeMap->setType(clone, paramtype);
                typeMap->setLeftValue(clone);
            }
        } else {
            argValue = newarg;
        }
        if (p->direction != IR::Direction::In && useTemp) {
            auto assign = new IR::AssignmentStatement(
                cloner.clone<IR::Expression>(newarg),
                cloner.clone<IR::Expression>(argValue));
            copyBack->push_back(assign);
            LOG3("Will copy out value " << dbp(assign));
        }
        args->push_back(new IR::Argument(arg->name, argValue));
    }

    // Special handling for table.apply(...).X; we cannot generate
    // a temporary for the method call tmp = table.apply(), since
    // we cannot write down the type of tmp.  So we don't
    // dismantle these expressions.
    bool tbl_apply = false;
    if (auto mmbr = getParent<IR::Member>()) {
        auto tbl = TableApplySolver::isActionRun(mmbr, refMap, typeMap);
        auto tbl1 = TableApplySolver::isHit(mmbr, refMap, typeMap);
        auto tbl2 = TableApplySolver::isMiss(mmbr, refMap, typeMap);
        tbl_apply = tbl != nullptr || tbl1 != nullptr || tbl2 != nullptr;
    }
    // Simplified method call, with arguments substituted
    if (!IR::equiv(mce->arguments, args))
        mce->arguments = args;
    typeMap->setType(mce, type);
    const IR::Expression *rv = mce;
    // See whether we assign the result of the call to a temporary
    if (type->is<IR::Type_Void>() ||               // no return type
        getParent<IR::MethodCallStatement>()) {    // result of call is not used
        statements.push_back(new IR::MethodCallStatement(mce->srcInfo, mce));
        rv = nullptr;
    } else if (tbl_apply) {
        typeMap->setType(mce, type);
        rv = mce;
    } else if (getParent<IR::AssignmentStatement>() && copyBack->empty()) {
        /* no need for an extra copy as there's no out args to copy back afterwards */
        typeMap->setType(mce, type);
        rv = mce;
    } else {
        auto tmp = createTemporary(type);
        auto left = new IR::PathExpression(IR::ID(tmp, nullptr));
        auto stat = new IR::AssignmentStatement(left, mce);
        statements.push_back(stat);
        rv = left->clone();
        typeMap->setType(rv, type);
        LOG3(orig << " replaced with " << left << " = " << mce);
    }
    statements.append(*copyBack);
    prune();
    return rv;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::Function* function) {
    if (toInsert.empty())
        return function;
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : function->body->components)
        body->push_back(s);
    function->body = body;
    toInsert.clear();
    return function;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Parser* parser) {
    if (toInsert.empty())
        return parser;
    parser->parserLocals.append(toInsert);
    toInsert.clear();
    return parser;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Control* control) {
    if (toInsert.empty())
        return control;
    control->controlLocals.append(toInsert);
    toInsert.clear();
    return control;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::P4Action* action) {
    if (toInsert.empty())
        return action;
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : action->body->components)
        body->push_back(s);
    action->body = body;
    toInsert.clear();
    return action;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ParserState* state) {
    if (state->selectExpression == nullptr)
        return state;
    state->components.append(statements);
    statements.clear();
    return state;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::AssignmentStatement* statement) {
    if (statements.empty())
        return statement;
    statements.push_back(statement);
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::MethodCallStatement* statement) {
    if (statements.empty()) {
        BUG_CHECK(statement->methodCall, "NULL methodCall?");
        return statement; }
    if (statement->methodCall)
        statements.push_back(statement);
    if (statements.size() == 1) {
        auto rv = statements.front();
        statements.clear();
        return rv; }
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node* DoSimplifyExpressions::postorder(IR::ReturnStatement* statement) {
    if (statements.empty())
        return statement;
    statements.push_back(statement);
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::IfStatement* statement) {
    IR::Statement *rv = statement;
    visit(statement->condition, "condition");
    if (!statements.empty()) {
        statements.push_back(statement);
        rv = new IR::BlockStatement(statements);
        statements.clear(); }
    visit(statement->ifTrue, "ifTrue");
    visit(statement->ifFalse, "ifFalse");
    prune();
    return rv;
}

const IR::Node* DoSimplifyExpressions::preorder(IR::SwitchStatement* statement) {
    IR::Statement *rv = statement;
    visit(statement->expression, "expression");
    if (!statements.empty()) {
        statements.push_back(statement);
        rv = new IR::BlockStatement(statements);
        statements.clear(); }
    visit(statement->cases, "cases");
    prune();
    return rv;
}

void DoSimplifyExpressions::end_apply(const IR::Node *) {
    BUG_CHECK(toInsert.empty(), "DoSimplifyExpressions::end_apply orphaned declarations");
    BUG_CHECK(statements.empty(), "DoSimplifyExpressions::end_apply orphaned statements");
}

}  // namespace P4
