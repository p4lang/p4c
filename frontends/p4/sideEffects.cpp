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
        // We cannot directly mutate v, because of https://github.com/p4lang/p4c/issues/43
        vec.push_back(new IR::NamedExpression(v->name, path));
    }
    expression->components = vec;
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
const IR::Parameter* DoSimplifyExpressions::findAlias(const IR::Parameter* param1,
                                                      const IR::Expression* expr1,
                                                      const IR::MethodCallExpression* mce1) {
    auto mi = MethodInstance::resolve(mce1, refMap, typeMap);
    const IR::Parameter* returnParam = nullptr;

    for (auto param2 : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg2 = mi->substitution.lookup(param2);

        auto expr2 = arg2->expression;
        if (auto mce2 = expr2->to<IR::MethodCallExpression>()) {
            returnParam =  findAlias(param1, expr1, mce2);
            continue;
        }
        if (param1->direction == IR::Direction::In && param2->direction == IR::Direction::In) {
            continue;
        }
        if (param1->type == param2->type && mayAlias(expr1, expr2)) {
            returnParam = param2;
            break;
        }
    }
    return returnParam;
}

cstring DoSimplifyExpressions::declareTemporary(const IR::Parameter* p,
                                                const IR::Expression* expr) {
    auto argType = typeMap->getType(p, true);
    if (argType->is<IR::Type_Dontcare>())
        argType = typeMap->getType(expr, true);

    auto tmp = createTemporary(argType);
    if (p->direction != IR::Direction::Out) {
        auto newArg = addAssignment(expr->srcInfo, tmp, expr);
        typeMap->setType(newArg, argType);
        typeMap->setLeftValue(newArg);
    }

    return tmp;
}
const IR::AssignmentStatement* DoSimplifyExpressions::copyOutValue(
    const IR::Expression* expr1,
    const IR::Expression* expr2,
    IR::IndexedVector<IR::StatOrDecl>* copyBack) {

    ClonePathExpressions cloner;
    auto assign = new IR::AssignmentStatement(
            cloner.clone<IR::Expression>(expr1),
            cloner.clone<IR::Expression>(expr2));
    copyBack->push_back(assign);

    return  assign;
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
    std::map<const IR::Argument*, const IR::Expression*> newArgsValues;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (p->direction == IR::Direction::None)
            continue;

        auto arg = mi->substitution.lookup(p);

        // If the parameter is out and the argument is a slice then
        // also use a temporary; makes the job of def-use analysis easier
        if (arg->expression->is<IR::Slice>() &&
            p->hasOut()) {
            LOG3("Using temporary for " << dbp(mce) <<
                 " param " << dbp(p) << " since it is an out slice");
            useTemporary.emplace(p);
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

    // Check aliasing between all pairs of arguments where at
    // least one of them is out or inout.
    for (auto p1 : *mi->substitution.getParametersInArgumentOrder()) {
        if (p1->direction == IR::Direction::None)
           continue;
        auto arg1 = mi->substitution.lookup(p1);

        for (auto p2 : *mi->substitution.getParametersInArgumentOrder()) {
            if (p2->direction == IR::Direction::None)
                continue;

            LOG3("p1=" << dbp(p1) << " p2=" << dbp(p2));
            if (p2 == p1)
                break;

            auto arg2 = mi->substitution.lookup(p2);

            if (auto m = arg1->expression->to<IR::Member>()) {
                auto u = arg1->expression->to<IR::Operation_Unary>();
                if  (u->expr->is<IR::ArrayIndex>()) {
                    auto b = u->expr->to<IR::Operation_Binary>();
                    // we handle this case T f(in T x, inout T y){...}; T g(inout T z){...}
                    // f(g(val), s[val].x)
                    // tmp_0 = val;
                    // tmp_1 = s[tmp_0].x;
                    // tmp_2 = g(val);
                    // f(tmp_2, tmp_1);
                    // s[tmp_0].x = tmp_1;
                    if (!b->right->is<IR::MethodCallExpression>() &&
                        arg2->expression->is<IR::MethodCallExpression>()) {
                        auto mce2 = arg2->expression->to<IR::MethodCallExpression>();
                        auto p3 = findAlias(p1, b->right, mce2);
                        if  (p3) {
                            auto mi2 = MethodInstance::resolve(mce2, refMap, typeMap);
                            auto arg3 = mi2->substitution.lookup(p3);
                            if (p3->hasOut() && p1->hasOut()) {
                                ::warning(ErrorType::WARN_ORDERING,
                                    "%1%: 'out' argument has fields in common with %2%",
                                    arg1, arg3);
                            }
                            // tmp_0 = val;
                            auto tmp0 = declareTemporary(p1, b->right);
                            const IR::Expression* argValue0 =
                                new IR::PathExpression(IR::ID(tmp0, nullptr));
                            // s[tmp_0].x
                            auto newArrayIndex = new IR::ArrayIndex(b->left, argValue0);
                            auto newMember = new IR::Member(newArrayIndex, m->member);
                            // tmp_1 = s[tmp_0].x;
                            auto tmp1 = declareTemporary(p1, newMember);
                            const IR::Expression* argValue1 =
                                new IR::PathExpression(IR::ID(tmp1, nullptr));

                            // s[tmp_0].x = tmp_1;
                            if (p1->direction != IR::Direction::In)
                                copyOutValue(newMember, argValue1, copyBack);

                            newArgsValues[arg1] = argValue1;
                        }
                    }
                    // we handle this case T f(in T x, inout T y){...}; T g(inout T z){...}
                    // f(val, s[g(val)].x)
                    // tmp_0 = val;
                    // tmp_1 = g(val);
                    // tmp_2 = s[tmp_1].x;
                    // f(tmp_0, tmp_2);
                    // s[tmp_1].x = tmp_2;
                    if (b->right->is<IR::MethodCallExpression>() &&
                        !arg2->expression->is<IR::MethodCallExpression>()) {
                        auto mce1 = b->right->to<IR::MethodCallExpression>();
                        auto p3 = findAlias(p2, arg2->expression, mce1);
                        if  (p3) {
                            auto mi1 = MethodInstance::resolve(mce1, refMap, typeMap);
                            auto arg3 = mi1->substitution.lookup(p3);
                            if (p3->hasOut() && p2->hasOut()) {
                                ::warning(ErrorType::WARN_ORDERING,
                                    "%1%: 'out' argument has fields in common with %2%",
                                    arg1, arg3);
                            }
                            // tmp_0 = val
                            auto tmp0 = declareTemporary(p2, arg2->expression);
                            const IR::Expression* argValue0 =
                                new IR::PathExpression(IR::ID(tmp0, nullptr));
                            // val = tmp_0
                            if (p2->direction != IR::Direction::In)
                                copyOutValue(arg2->expression, argValue0, copyBack);
                            // tmp_1 = g(val)
                            auto tmp1 = declareTemporary(p1, b->right);
                            const IR::Expression* argValue1 =
                                new IR::PathExpression(IR::ID(tmp1, nullptr));
                            // s[tmp_1].x
                            auto newArrayIndex = new IR::ArrayIndex(b->left, argValue1);
                            auto newMember = new IR::Member(newArrayIndex, m->member);
                            // tmp_2 = s[tmp_1].x;
                            auto tmp2 = declareTemporary(p1, newMember);
                            const IR::Expression* argValue2 =
                                new IR::PathExpression(IR::ID(tmp2, nullptr));
                            // s[tmp_1].x = tmp_2
                            if (p1->direction != IR::Direction::In)
                                copyOutValue(newMember, argValue2, copyBack);
                            // save new temporaries as argument of f
                            // f(arg2, arg1) ---> f(tmp_0, tmp_2)
                            newArgsValues[arg1] = argValue2;
                            newArgsValues[arg2] = argValue0;
                        }
                    }
                    continue;
                }
            }

            if (auto m = arg2->expression->to<IR::Member>()) {
                auto u = arg2->expression->to<IR::Operation_Unary>();
                if  (u->expr->is<IR::ArrayIndex>()) {
                    auto b = u->expr->to<IR::Operation_Binary>();
                    // we handle this case T f(inout T x, in T y) {...}; T g(inout T z){...}
                    // f(s[val].x, g(val))
                    // tmp_0 = val;
                    // tmp_1 = s[tmp_0].x;
                    // tmp_2 = g(val);
                    // f(tmp_1, tmp_2);
                    // s[tmp_0].x = tmp_1;
                    if (!b->right->is<IR::MethodCallExpression>() &&
                        arg1->expression->is<IR::MethodCallExpression>()) {
                        auto mce1 = arg1->expression->to<IR::MethodCallExpression>();
                        auto p3 = findAlias(p2, b->right, mce1);
                        if  (p3) {
                            auto mi1 = MethodInstance::resolve(mce1, refMap, typeMap);
                            auto arg3 = mi1->substitution.lookup(p3);
                            if (p3->hasOut() && p2->hasOut()) {
                                 ::warning(ErrorType::WARN_ORDERING,
                                    "%1%: 'out' argument has fields in common with %2%",
                                     arg2, arg3);
                            }
                            // tmp_0 = val;
                            auto tmp0 = declareTemporary(p2, b->right);
                            const IR::Expression* argValue0 =
                                new IR::PathExpression(IR::ID(tmp0, nullptr));
                            // s[tmp_0].x
                            auto newArrayIndex = new IR::ArrayIndex(b->left, argValue0);
                            auto newMember = new IR::Member(newArrayIndex, m->member);
                            // tmp_1 = s[tmp_0].x;
                            auto tmp1 = declareTemporary(p2, newMember);
                            const IR::Expression* argValue1 =
                                new IR::PathExpression(IR::ID(tmp1, nullptr));
                            // s[tmp_0].x = tmp_1
                            if (p2->direction != IR::Direction::In)
                                copyOutValue(newMember, argValue1, copyBack);

                            newArgsValues[arg2] = argValue1;
                        }
                    }
                    // we handle this case T f(inout T x, in T y); T g(inout T z) {...}
                    // f(s[g(val)].x, val)
                    // tmp_0 = val;
                    // tmp_1 = g(val);
                    // tmp_2 = s[tmp_1].x;
                    // f(tmp_2, tmp_0);
                    // s[tmp_1].x = tmp_2;
                    if (b->right->is<IR::MethodCallExpression>() &&
                        !arg1->expression->is<IR::MethodCallExpression>()) {
                        auto mce1 = b->right->to<IR::MethodCallExpression>();
                        auto p3 = findAlias(p1, arg1->expression, mce1);
                        if  (p3) {
                            auto mi1 = MethodInstance::resolve(mce1, refMap, typeMap);
                            auto arg3 = mi1->substitution.lookup(p3);
                            if (p3->hasOut() && p1->hasOut()) {
                                 ::warning(ErrorType::WARN_ORDERING,
                                    "%1%: 'out' argument has fields in common with %2%",
                                     arg1, arg3);
                            }
                            // tmp_0 = val
                            auto tmp0 = declareTemporary(p3, arg3->expression);
                            const IR::Expression* argValue0 =
                                new IR::PathExpression(IR::ID(tmp0, nullptr));
                            // val = tmp_0
                            if (p1->direction != IR::Direction::In)
                                copyOutValue(arg3->expression, argValue0, copyBack);
                            // tmp_1 = g(val)
                            auto tmp1 = declareTemporary(p2, b->right);
                            const IR::Expression* argValue1 =
                                new IR::PathExpression(IR::ID(tmp1, nullptr));
                            // s[tmp_1].x
                            auto newArrayIndex = new IR::ArrayIndex(b->left, argValue1);
                            auto newMember = new IR::Member(newArrayIndex, m->member);
                            // tmp_2 = s[tmp_1].x
                            auto tmp2 = declareTemporary(p2, newMember);
                            const IR::Expression* argValue2 =
                                new IR::PathExpression(IR::ID(tmp2, nullptr));
                            // s[tmp_1].x = tmp_2
                            if (p2->direction != IR::Direction::In)
                                copyOutValue(newMember, argValue2, copyBack);
                            // save new temporaries as argument of f
                            // f(arg2, arg1) ---> f(tmp_2, tmp_0)
                            newArgsValues[arg1] = argValue0;
                            newArgsValues[arg2] = argValue2;
                        }
                    }
                    continue;
                }
            }

            // not handle this case f(g(val), h(val))
            if (arg1->expression->is<IR::MethodCallExpression>() &&
                arg2->expression->is<IR::MethodCallExpression>()) {
                continue;
            }

            // we handle this case T f(inout T a, in T b) {...}; T g(inout T c){...}
            // f(val, g(val))
            // tmp = val
            // tmp_0 = g(val)
            // f(tmp, tmp_0)
            // val = tmp
            if (arg1->expression->is<IR::MethodCallExpression>()) {
                auto mce1 = arg1->expression->to<IR::MethodCallExpression>();
                auto p3 = findAlias(p2, arg2->expression, mce1);
                if  (p3) {
                    auto mi1 = MethodInstance::resolve(mce1, refMap, typeMap);
                    auto arg3 = mi1->substitution.lookup(p3);
                    if (p3->hasOut() && p2->hasOut()) {
                        ::warning(ErrorType::WARN_ORDERING,
                            "%1%: 'out' argument has fields in common with %2%",
                            arg2, arg3);
                    }
                    useTemporary.emplace(p3);
                    useTemporary.emplace(p2);
                }
                continue;
            }
            // handle this case f(in T a, inout T b){...}; T g(inout T c){...}
            // f(g(val), val)
            // tmp = val
            // tmp_0 = g(val)
            // f(tmp_0, tmp)
            // val = tmp
            if (arg2->expression->is<IR::MethodCallExpression>()) {
                auto mce2 = arg2->expression->to<IR::MethodCallExpression>();
                auto p3 = findAlias(p1, arg1->expression, mce2);
                if  (p3) {
                    auto mi2 = MethodInstance::resolve(mce2, refMap, typeMap);
                    auto arg3 = mi2->substitution.lookup(p3);
                    if (p3->hasOut() && p1->hasOut()) {
                                 ::warning(ErrorType::WARN_ORDERING,
                                    "%1%: 'out' argument has fields in common with %2%",
                                     arg1, arg3);
                    }
                    // tmp = val
                    auto tmp1 = declareTemporary(p1, arg1->expression);
                    const IR::Expression* argValue1 =
                        new IR::PathExpression(IR::ID(tmp1, nullptr));

                    // val = tmp
                    if  (p1->direction != IR::Direction::In)
                        copyOutValue(arg1->expression, argValue1, copyBack);
                    // save new temporary as argument of f
                    newArgsValues[arg1] = argValue1;
                }
                continue;
            }
            if (p1->direction == IR::Direction::In && p2->direction == IR::Direction::In) {
                continue;
            }
            // and handle this case f(val, val)
            if (p1->type == p2->type && mayAlias(arg1->expression, arg2->expression)) {
                LOG3("Using temporary for " << dbp(mce) <<
                     " param " << dbp(p1) << " aliasing" << dbp(p2));
                if (p1->hasOut() && p2->hasOut())
                    ::warning(ErrorType::WARN_ORDERING,
                              "%1%: 'out' argument has fields in common with %2%", arg1, arg2);
                useTemporary.emplace(p1);
                useTemporary.emplace(p2);
            }
        }
    }

    visit(mce->method);

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
        if (newArgsValues[arg] == nullptr)
            visit(arg);  // May mutate arg!  Recursively simplifies arg.
        auto newarg = arg->expression;
        CHECK_NULL(newarg);

        if (useTemp) {
            // declare temporary variable
            // tmp = arg->expression
            auto tmp = declareTemporary(p, arg->expression);
            argValue = new IR::PathExpression(IR::ID(tmp, nullptr));
            // arg->expresion = tmp
            if  (p->direction != IR::Direction::In)
                copyOutValue(arg->expression, argValue, copyBack);
        } else {
            argValue = newarg;
        }
        if (newArgsValues[arg] == nullptr) {
            args->push_back(new IR::Argument(arg->name, argValue));
        } else {
            args->push_back(new IR::Argument(arg->name, newArgsValues[arg]));
        }
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
